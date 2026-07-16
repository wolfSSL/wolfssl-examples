#!/usr/bin/env python3
"""Triage a nightly run: retry once, classify by retry outcome, file issues.

Doctrine (from wolfProvider, and the reason this works at all):

    Cleared on retry = flake. Failed twice = real. A reproducing failure is REAL
    even if low-impact -- that is what severity is for. "flake" means ONLY a
    transient infra/network hiccup.

Classification is by RETRY OUTCOME, never by guessing from the log.
"""

import json
import os
import re
import sys
import time
import traceback
import urllib.error
import urllib.request
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone

import issue_store as store

REPO = os.environ["GH_REPO"]
RUN_ID = os.environ["RUN_ID"]
TOKEN = os.environ["GITHUB_TOKEN"]
ANTHROPIC_KEY = os.environ.get("ANTHROPIC_API_KEY", "")
CLAUDE_MODEL = os.environ.get("CLAUDE_MODEL", "claude-opus-4-8")
AUTO_RETRY = os.environ.get("AUTO_RETRY", "false") == "true"
DRY_RUN = os.environ.get("DRY_RUN", "true") == "true"
RETRY_DEADLINE_S = int(os.environ.get("RETRY_DEADLINE_S", "3600"))

API = "https://api.github.com"
POLL_INTERVAL_S = 30
JOBS_PER_PAGE = 50  # the jobs endpoint 502s at per_page=100 on large runs
AI_MAX_CALLS = 15
LOG_TAIL_CHARS = 4000


def gh(path, method="GET", body=None, retry=True):
    url = path if path.startswith("http") else f"{API}{path}"
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(url, method=method, data=data)
    req.add_header("Authorization", f"Bearer {TOKEN}")
    req.add_header("Accept", "application/vnd.github+json")
    if data:
        req.add_header("Content-Type", "application/json")
    attempts = 3 if retry else 1
    for i in range(attempts):
        try:
            with urllib.request.urlopen(req, timeout=60) as r:
                raw = r.read()
                return json.loads(raw) if raw else {}
        except urllib.error.HTTPError:
            raise
        except Exception:
            if i == attempts - 1:
                raise
            time.sleep(2 ** i)


def all_jobs(run_id, attempt=None):
    base = f"/repos/{REPO}/actions/runs/{run_id}"
    base += f"/attempts/{attempt}/jobs" if attempt else "/jobs"
    jobs, page = [], 1
    while True:
        got = gh(f"{base}?per_page={JOBS_PER_PAGE}&page={page}").get("jobs", [])
        jobs += got
        if len(got) < JOBS_PER_PAGE:
            return jobs
        page += 1


# --- retry: poll, do not trampoline ----------------------------------------
#
# wolfProvider POSTs rerun-failed-jobs and returns, relying on the rerun
# re-firing workflow_run. Three paths then produce a GREEN triage run with zero
# output: the rerun never schedules; a 5xx on the POST raises out of main(); and
# the GITHUB_TOKEN loophole it leans on is undocumented behavior used as control
# flow. Polling dissolves all three. There is deliberately no path here that
# returns without the caller going on to report.


def rerun_and_wait(run_id):
    before = int(gh(f"/repos/{REPO}/actions/runs/{run_id}").get("run_attempt", 1))
    try:
        gh(f"/repos/{REPO}/actions/runs/{run_id}/rerun-failed-jobs", method="POST")
    except urllib.error.HTTPError as e:
        # 403 = run too old / Actions disabled / "No failed jobs" (every non-pass
        # was `cancelled`, which this endpoint will not rerun). 409 = already running.
        return before, f"retry could not be started (HTTP {e.code})"
    except Exception as e:
        return before, f"retry could not be started ({type(e).__name__})"

    end = time.monotonic() + RETRY_DEADLINE_S
    seen = before
    while time.monotonic() < end:
        time.sleep(POLL_INTERVAL_S)
        try:
            run = gh(f"/repos/{REPO}/actions/runs/{run_id}")
        except Exception:
            continue
        seen = int(run.get("run_attempt", before))
        # ORDER MATTERS: right after the POST the run still reads
        # completed/attempt=1. Checking status first returns on the first poll
        # and reports attempt 1 as though the retry had run.
        if seen <= before:
            continue
        if run.get("status") == "completed":
            return seen, ""
    if seen <= before:
        return before, "retry never started; reporting first-attempt results"
    return seen, f"retry did not finish within {RETRY_DEADLINE_S // 60}m; partial results"


# --- results ---------------------------------------------------------------


class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *a, **kw):
        return None


_NOREDIR = urllib.request.build_opener(_NoRedirect)


def fetch_artifact(url):
    """Download an artifact zip.

    The endpoint 302s to a pre-signed blob URL. urllib replays our Authorization
    header onto it, the blob store then tries to authenticate with a GitHub token
    and 401s -- so catch the redirect and fetch the Location with no auth.
    Getting this wrong empties every result, which reads as "nothing failed".
    """
    req = urllib.request.Request(url)
    req.add_header("Authorization", f"Bearer {TOKEN}")
    try:
        with _NOREDIR.open(req, timeout=120) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        if e.code not in (301, 302, 303, 307, 308):
            raise
        loc = e.headers.get("Location")
        if not loc:
            raise
        with urllib.request.urlopen(loc, timeout=120) as r:
            return r.read()


def fetch_results(run_id, attempt):
    """Per-dir results from the shard artifacts of one attempt."""
    # Paginate: one artifact per (example, ref) means a nightly with 3 refs and
    # 62 entries produces ~186. Reading page 1 only silently drops the tail, and a
    # dropped example is never triaged and never has its issue closed.
    arts, page = [], 1
    while True:
        got = gh(
            f"/repos/{REPO}/actions/runs/{run_id}/artifacts"
            f"?per_page={JOBS_PER_PAGE}&page={page}"
        ).get("artifacts", [])
        arts += got
        if len(got) < JOBS_PER_PAGE:
            break
        page += 1
    want = f"-attempt{attempt}"
    out = []
    wanted = read = 0
    for a in arts:
        if not a["name"].endswith(want) or a.get("expired"):
            continue
        wanted += 1
        try:
            import io
            import zipfile

            blob = fetch_artifact(a["archive_download_url"])
            read += 1
            with zipfile.ZipFile(io.BytesIO(blob)) as z:
                for name in z.namelist():
                    if name.endswith(".json"):
                        out += json.loads(z.read(name))
        except Exception as e:
            print(f"warn: could not read artifact {a['name']}: {e}", file=sys.stderr)
    # One unreadable artifact is a warning; none of them readable means triage is
    # blind, and reporting "0 failures" from that is the worst thing this can do.
    if wanted and not read:
        sys.exit(
            f"could not read ANY of {wanted} result artifact(s) for attempt "
            f"{attempt}. Refusing to report -- an empty read is not a pass."
        )
    return out


def merged_results(run_id, attempt):
    """Attempt 2 only reran the failed shards, so merge it OVER attempt 1."""
    # The ref MUST be in the key: nightly runs each example once per wolfSSL ref,
    # so keying on (id, target) alone collapses master and stable into one row and
    # a pass on one ref silently overwrites a failure on the other.
    def key(r):
        return (r["id"], r.get("ref", ""), r.get("target", ""))

    first = {key(r): r for r in fetch_results(run_id, 1)}
    if attempt >= 2:
        for r in fetch_results(run_id, attempt):
            first[key(r)] = r
    return list(first.values())


def classify(run_id, attempt, retry_note):
    """Fold per-target results into a per-dir observation."""
    a1 = defaultdict(list)
    for r in fetch_results(run_id, 1):
        a1[r["id"]].append(r["status"])
    final = defaultdict(list)
    for r in merged_results(run_id, attempt):
        final[r["id"]].append(r["status"])

    # Which refs actually failed. The caller used to hardcode "master", which made
    # the issue table and the AI prompt lie whenever a break was stable-only.
    fail_refs = defaultdict(set)
    for r in merged_results(run_id, attempt):
        if r["status"] == "fail":
            fail_refs[r["id"]].add(r.get("ref") or "unknown")
    # A flake has no failing ref in the final attempt by definition, so without
    # this its issue would name no ref at all. The ref that failed first is the
    # whole point of the report.
    a1_refs = defaultdict(set)
    for r in fetch_results(run_id, 1):
        if r["status"] == "fail":
            a1_refs[r["id"]].add(r.get("ref") or "unknown")

    obs = {}
    for eid in set(a1) | set(final):
        failed_now = "fail" in final.get(eid, [])
        failed_before = "fail" in a1.get(eid, [])
        if failed_now:
            obs[eid] = store.UNCONFIRMED if retry_note else store.REAL
        elif failed_before and attempt >= 2:
            obs[eid] = store.FLAKE_ONLY
            fail_refs[eid] |= a1_refs.get(eid, set())
        else:
            obs[eid] = store.PASS
    # every host dir came from examples.yml, which nightly.yml calls as `host`
    targets = {eid: "examples" for eid in obs}

    jobs_obs, jobs_refs, jobs_targets = classify_jobs(run_id, attempt, retry_note)
    obs.update(jobs_obs)
    fail_refs.update(jobs_refs)
    targets.update(jobs_targets)
    return obs, {k: sorted(v) for k, v in fail_refs.items()}, targets


# Jobs the nightly fans out to that emit no results-*.json. Without this they
# ran every night, went red, and told nobody -- 12 of the 13 targets.
# lowercased before matching: a `name:` override in nightly.yml renames every
# job under it, and a case mismatch would silently make "host" a 13th target
HOST_JOBS = {"host", "refs", "gate"}
REF_RE = re.compile(r"wolfSSL (\S+)|\((master|v[\d.]+-stable)\)")


def job_unit(name):
    """`esp32 / Build / ESP32 (...) wolfSSL master` -> ('esp32', 'master').

    The prefix is nightly.yml's job name, which is deliberately the target name
    and therefore the ci:<target> label.
    """
    if " / " not in name:
        return None, None
    target, rest = name.split(" / ", 1)
    if target.lower() in HOST_JOBS:
        return None, None
    m = REF_RE.search(rest)
    return target, (m.group(1) or m.group(2)) if m else "unknown"


HOST_JOB_RE = re.compile(r"^host / (?:Run|Build) / (.+) \((.+)\)$", re.I)


def job_urls(run_id):
    """(unit, ref) -> the URL of the job that ran it.

    Linking the run only makes a reader hunt through 250 jobs for the red one.
    """
    out = {}
    for j in all_jobs(run_id):
        name, url = j.get("name", ""), j.get("html_url")
        m = HOST_JOB_RE.match(name)
        if m:
            out[(m.group(1), m.group(2))] = url
            continue
        t, ref = job_unit(name)
        if t:
            out[(t, ref)] = url
    return out


def classify_jobs(run_id, attempt, retry_note):
    def by_unit(jobs):
        out = defaultdict(list)
        for j in jobs:
            t, ref = job_unit(j.get("name", ""))
            if t:
                out[t].append((j.get("conclusion"), ref))
        return out

    a1 = by_unit(all_jobs(run_id, attempt=1))
    final = by_unit(all_jobs(run_id)) if attempt >= 2 else a1

    obs, refs, targets = {}, defaultdict(set), {}
    for t in set(a1) | set(final):
        failed_now = any(c == "failure" for c, _ in final.get(t, []))
        failed_before = any(c == "failure" for c, _ in a1.get(t, []))
        if failed_now:
            obs[t] = store.UNCONFIRMED if retry_note else store.REAL
            refs[t] = {r for c, r in final[t] if c == "failure"}
        elif failed_before and attempt >= 2:
            obs[t] = store.FLAKE_ONLY
            # nothing failed in the final attempt, so name the ref that failed first
            refs[t] = {r for c, r in a1[t] if c == "failure"}
        else:
            obs[t] = store.PASS
        targets[t] = t
    return obs, refs, targets


# --- AI: advisory only -----------------------------------------------------
#
# N per-dir calls, not one batched call. Batching gives ~120 output tokens per
# job and makes one truncation invalidate every dir's verdict. Per-dir isolation
# is structural rather than a plea in the prompt.

SYSTEM = """\
You triage failures in the wolfSSL/wolfssl-examples nightly CI.

Context you must assume:
- Each example directory builds small C programs against a wolfSSL library built
  from a given git ref (master, or a stable tag) and installed to /usr/local.
- The job below failed TWICE: an automatic retry did not clear it. Infra setup
  failures are filtered out before you see this.
- You are judging ONE example directory from ONE log. You have no other context.

Failure taxonomy for this repo, roughly by frequency:
- missing-build-option: the example needs a wolfSSL ./configure flag that was not
  enabled ("WOLFSSL_KEY_GEN not defined", implicit declaration of a wc_* function).
  The example is fine; the profile is wrong. Usually Medium.
- api-drift: a wolfSSL API changed and the example was not updated (wrong argument
  count, removed symbol). The example must be fixed. High.
- link-error: "undefined reference to wc_*" - usually a build-option problem
  wearing a link error's clothes.
- cert-expiry: the shared certs/ fixtures expired (ASN_AFTER_DATE_E, -155). Affects
  the whole repo, not this dir. High, and blast is repo-wide.
- identity-gate: CI refused to run because a libwolfssl it did not build was
  visible. An infrastructure fault, not an example bug.
- runtime-failure: builds clean, then asserts / segfaults / returns non-zero.
- toolchain: missing header or library, compiler version incompatibility.
- infra-flake: apt/git/download timeout, DNS failure, disk full.

Verdict rules - not suggestions:
- "flake" ONLY if THIS log shows a transient network/registry/download hiccup.
- A build or test that reproducibly FAILS is "real" even if cosmetic or unrelated
  to crypto. Severity is where you express that. Never use "flake" to mean
  "unimportant".
- If the log is truncated or you cannot tell, say so in `symptom`, return "real"
  with severity Medium. Do not guess.

Set `upstream` true when the evidence says this is wolfSSL's regression rather than
the example's - typically api-drift that fails only against master while stable
refs pass. That routes a human to file against wolfSSL/wolfssl instead of here.

Do not speculate about outages or other jobs. Judge only this log. Every free-text
field is one line, plain, no markdown.
"""

SCHEMA = {
    "type": "object",
    "properties": {
        "verdict": {"type": "string", "enum": ["real", "flake"]},
        "category": {
            "type": "string",
            "enum": [
                "missing-build-option",
                "api-drift",
                "link-error",
                "cert-expiry",
                "identity-gate",
                "runtime-failure",
                "toolchain",
                "infra-flake",
                "unknown",
            ],
        },
        "severity": {"type": "string", "enum": ["Critical", "High", "Medium", "Low"]},
        "symptom": {"type": "string"},
        "cause": {"type": "string"},
        "next": {"type": "string"},
        "blast": {"type": "string", "enum": ["this-dir", "repo-wide"]},
        "upstream": {"type": "boolean"},
    },
    "required": [
        "verdict",
        "category",
        "severity",
        "symptom",
        "cause",
        "next",
        "blast",
        "upstream",
    ],
    "additionalProperties": False,
}

TIER_SEV = {"build": "High", "run": "Medium", "setup": "Medium"}


def ai_one(client, fd):
    if client is None:
        return None
    user = (
        f"example dir: {fd['id']}\n"
        f"failing refs: {', '.join(fd['refs'])}\n"
        f"consecutive nights failing: {fd['streak']}\n\n"
        f"log tail (secrets scrubbed):\n{store.scrub(fd['log'])[-LOG_TAIL_CHARS:]}"
    )
    try:
        resp = client.messages.create(
            model=CLAUDE_MODEL,
            max_tokens=2000,
            system=SYSTEM,
            thinking={"type": "adaptive"},
            output_config={
                "effort": "low",
                "format": {"type": "json_schema", "schema": SCHEMA},
            },
            messages=[{"role": "user", "content": user}],
        )
        if resp.stop_reason in ("refusal", "max_tokens"):
            return None
        text = next((b.text for b in resp.content if b.type == "text"), "")
        return json.loads(text)
    except Exception as e:
        print(f"warn: AI triage unavailable for {fd['id']}: {e}", file=sys.stderr)
        return None


def ai_batch(failures):
    if not ANTHROPIC_KEY or not failures:
        return {}
    if len(failures) > AI_MAX_CALLS:
        print(
            f"note: {len(failures)} failures exceeds the {AI_MAX_CALLS}-call cap; "
            "this looks systemic, skipping AI triage",
            file=sys.stderr,
        )
        return {}
    try:
        import anthropic

        client = anthropic.Anthropic(api_key=ANTHROPIC_KEY)
    except ImportError:
        print("warn: anthropic SDK not installed; skipping AI triage", file=sys.stderr)
        return {}
    with ThreadPoolExecutor(max_workers=4) as pool:
        verdicts = list(pool.map(lambda f: ai_one(client, f), failures))
    return {f["id"]: v for f, v in zip(failures, verdicts) if v}


# --- issues ----------------------------------------------------------------


def open_issues():
    """REST list, not Search: strongly consistent and returns bodies."""
    out, page = [], 1
    while True:
        got = gh(
            f"/repos/{REPO}/issues?labels={store.LABEL}&state=all"
            f"&per_page=100&page={page}"
        )
        out += [i for i in got if "pull_request" not in i]
        if len(got) < 100:
            return out
        page += 1


NIGHTLY_TARGETS = ("examples", "esp32", "android", "rt1060", "uefi", "puf",
                   "rpi-pico", "ebpf", "fullstack", "java", "cmake", "csharp",
                   "emulated")


def ensure_labels():
    for t in NIGHTLY_TARGETS:
        try:
            gh(f"/repos/{REPO}/labels", method="POST", retry=False,
               body={"name": store.target_label(t), "color": "1d76db",
                     "description": f"Nightly {t} target"})
        except urllib.error.HTTPError:
            pass  # 422 = already exists
    for name, color, desc in [
        (store.LABEL, "b60205", "Filed by the nightly examples triage"),
        (store.LABEL_MUTE, "ededed", "Triage will not touch this issue"),
        (store.LABEL_UNCONFIRMED, "d4c5f9", "Retry did not run; not double-confirmed"),
    ]:
        try:
            gh(
                f"/repos/{REPO}/labels",
                method="POST",
                body={"name": name, "color": color, "description": desc},
                retry=False,
            )
        except urllib.error.HTTPError:
            pass  # 422 = already exists


def previous_job_count():
    """Baseline for the circuit breaker: the last COMPLETED nightly's job count."""
    try:
        runs = gh(
            f"/repos/{REPO}/actions/workflows/nightly.yml/runs"
            f"?status=completed&per_page=2"
        ).get("workflow_runs", [])
        for r in runs:
            if str(r["id"]) != str(RUN_ID):
                return len(all_jobs(r["id"]))
    except Exception:
        pass
    return 0


def run_url():
    return f"https://github.com/{REPO}/actions/runs/{RUN_ID}"


def apply(eid, observation, issue, ai, state, rows, close_allowed, target):
    """Returns the issue number now tracking this dir, or None."""
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    action, why = store.decide(observation, issue, close_allowed)
    num = (issue or {}).get("number")

    if action == "noop":
        print(f"  {eid}: noop ({why})")
        return num
    if DRY_RUN:
        print(f"  {eid}: WOULD {action} ({why})")
        return num

    body = store.render_body(eid, state, ai, rows, run_url(), now)

    if action == "open":
        body_kw = {
                "title": f"CI: `{eid}` failing in the nightly",
                "body": body,
                "labels": [store.LABEL, store.target_label(target)]
                + ([store.LABEL_UNCONFIRMED] if observation == store.UNCONFIRMED else []),
                # wolfHSM assigns its nightly issues to the bot; retried
                # without it below if the account is not assignable here.
                "assignees": ["wolfSSL-Bot"],
        }
        try:
            created = gh(f"/repos/{REPO}/issues", method="POST", body=body_kw)
        except urllib.error.HTTPError:
            # not assignable in this repo; file it anyway rather than lose it
            body_kw.pop("assignees", None)
            created = gh(f"/repos/{REPO}/issues", method="POST", body=body_kw)
        num = created["number"]
        print(f"  {eid}: opened #{num} ({why})")
    elif action == "reopen":
        gh(
            f"/repos/{REPO}/issues/{issue['number']}",
            method="PATCH",
            body={"state": "open", "body": body},
        )
        gh(
            f"/repos/{REPO}/issues/{issue['number']}/comments",
            method="POST",
            body={"body": f"Failing again in the nightly. [Run]({run_url()})"},
        )
        print(f"  {eid}: reopened #{issue['number']} ({why})")
    elif action == "update":
        gh(f"/repos/{REPO}/issues/{issue['number']}", method="PATCH", body={"body": body})
        # A comment every night, so the thread IS the history. wolfHSM #412 does
        # this with a bare "Still failing as of <url>" x15; carry the verdict,
        # the retry outcome and what changed instead, so a reader can skim the
        # thread and see how the failure evolved.
        reason = store.material_change(store.parse_state(issue.get("body", "")), state)
        gh(
            f"/repos/{REPO}/issues/{issue['number']}/comments",
            method="POST",
            body={"body": store.render_comment(state, ai, observation, reason,
                                               run_url(), now)},
        )
        print(f"  {eid}: updated #{issue['number']} + comment"
              + (f" ({reason})" if reason else ""))
    elif action == "close":
        gh(
            f"/repos/{REPO}/issues/{issue['number']}/comments",
            method="POST",
            body={"body": f"{why.capitalize()} — closing. [Run]({run_url()})"},
        )
        gh(
            f"/repos/{REPO}/issues/{issue['number']}",
            method="PATCH",
            body={"state": "closed", "state_reason": "completed"},
        )
        print(f"  {eid}: closed #{issue['number']} ({why})")
        num = None

    return num


def report(attempt, retry_note):
    results = merged_results(RUN_ID, attempt)
    obs, fail_refs, targets = classify(RUN_ID, attempt, retry_note)

    logs = defaultdict(str)
    for r in results:
        if r["status"] == "fail" and r.get("log"):
            logs[r["id"]] += r["log"]

    issues = {store.parse_fp(i.get("body", "")): i for i in open_issues()}
    issues.pop(None, None)

    urls = job_urls(RUN_ID)
    total = len(all_jobs(RUN_ID))
    prev = previous_job_count()
    close_allowed = not (prev and total < 0.5 * prev)
    if not close_allowed:
        print(
            f"CIRCUIT BREAKER: {total} jobs vs {prev} last nightly (<50%). "
            "Refusing to close anything this run.",
            file=sys.stderr,
        )

    failing = [e for e, o in obs.items() if o in (store.REAL, store.UNCONFIRMED)]
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d")

    ai_input = []
    for eid in failing:
        old = store.parse_state(issues.get(eid, {}).get("body", ""))
        ai_input.append(
            {
                "id": eid,
                "refs": fail_refs.get(eid, []),
                "streak": old.get("streak", 0) + 1,
                "log": logs.get(eid, "(no log captured)"),
            }
        )
    verdicts = ai_batch(ai_input)

    if not DRY_RUN:
        ensure_labels()

    print(f"\n{len(failing)} real failure(s), attempt {attempt}"
          f"{' — ' + retry_note if retry_note else ''}\n")

    filed = {}
    for eid, observation in sorted(obs.items()):
        issue = issues.get(eid)
        old = store.parse_state(issue.get("body", "")) if issue else {}
        sig = (
            store.signature(logs[eid])
            if logs.get(eid)
            else old.get("sig", "unknown")
        )
        opens = old.get("opens", 0) + (1 if not issue or issue.get("state") == "closed" else 0)
        sticky = old.get("sticky", False) or opens >= 3
        state = {
            "opens": opens,
            "first": old.get("first", now),
            "last_fail": now,
            "streak": old.get("streak", 0) + 1 if observation != store.PASS else 0,
            "sig": sig,
            "sticky": sticky,
            # Written, not just read: without this a sticky issue re-reports
            # "flapping" every night -- the exact spam material_change forbids.
            "flaky_noted": old.get("flaky_noted", False) or sticky,
            "failing": fail_refs.get(eid, []) if observation != store.PASS else [],
            "log": logs.get(eid, "")[-LOG_TAIL_CHARS:],
        }
        # The real failing target + its error + a link to the job that ran it.
        # This used to be a hardcoded "-" profile column and a bare ":x: fail",
        # which told a reader nothing they could act on.
        rows = [
            {
                "test": r.get("target") or r.get("stage", "build"),
                "ref": r.get("ref", "?"),
                "detail": (r.get("detail") or "").strip(),
                "url": urls.get((eid, r.get("ref"))),
            }
            for r in results
            if r["id"] == eid and r["status"] == "fail"
        ]
        # cross targets emit no results rows, so fall back to the job itself
        if not rows and observation != store.PASS:
            rows = [
                {"test": eid, "ref": ref, "detail": "job failed",
                 "url": urls.get((eid, ref))}
                for ref in fail_refs.get(eid, ["unknown"])
            ]
        # collected, not read back from `issues`: a dir filed for the first time
        # tonight is not in that map, and its card entry would have no link
        filed[eid] = apply(eid, observation, issue, verdicts.get(eid), state, rows,
                           close_allowed, targets.get(eid, "examples"))


def main():
    run = gh(f"/repos/{REPO}/actions/runs/{RUN_ID}")
    attempt = int(run.get("run_attempt", 1))
    retry_note = ""

    if AUTO_RETRY and attempt == 1:
        nonpass = [
            j for j in all_jobs(RUN_ID) if j.get("conclusion") in ("failure", "cancelled")
        ]
        if nonpass:
            print(f"{len(nonpass)} non-passing job(s); retrying once and waiting...")
            attempt, retry_note = rerun_and_wait(RUN_ID)
            if retry_note:
                print(f"note: {retry_note}", file=sys.stderr)

    try:
        report(attempt, retry_note)
    except Exception:
        # A broken reporter must not be a silent reporter.
        print("TRIAGE CRASHED:\n" + traceback.format_exc(), file=sys.stderr)
        raise


if __name__ == "__main__":
    main()
