#!/usr/bin/env python3
"""Issue identity, body rendering, and the open/close state machine.

Identity is an HTML marker matched in-process against the REST *list* endpoint,
never the Search API: `is:issue in:body` is eventually consistent, so a search
that misses files a duplicate. `GET /issues?labels=...&state=open` hits the
primary DB, is strongly consistent, and returns bodies.
"""

import hashlib
import json
import re

LABEL = "ci:nightly"
# ci:<target> -- the nightly job that failed (examples, esp32, android, ...).
# Added only when that target fails, so a green night adds no labels.
def target_label(target):
    return f"ci:{target}"

LABEL_MUTE = "ci:mute"
LABEL_UNCONFIRMED = "ci:unconfirmed"

FP_RE = re.compile(r"<!--\s*ci-fp:\s*v1\|([^\s>]+)\s*-->")
STATE_RE = re.compile(r"<!--\s*ci-state:\s*(\{.*?\})\s*-->", re.S)

# Secrets are scrubbed before anything reaches a PUBLIC issue tracker -- both
# log tails and every AI-authored field (a model can quote a secret back).
SCRUB = [
    (re.compile(r"gh[pousr]_[A-Za-z0-9]{20,}"), "***token***"),
    (re.compile(r"(?i)bearer\s+[A-Za-z0-9._\-]+"), "Bearer ***"),
    (
        re.compile(
            r"(?i)(x-api-key|authorization|password|secret|token)(['\"\s:=]+)\S+"
        ),
        r"\1\2***",
    ),
    (
        re.compile(r"eyJ[A-Za-z0-9_\-]{8,}\.[A-Za-z0-9_\-]{8,}\.[A-Za-z0-9_\-]{8,}"),
        "***jwt***",
    ),
    (re.compile(r"sk-ant-[A-Za-z0-9_\-]{10,}"), "***anthropic-key***"),
]


def scrub(text):
    if not text:
        return text
    for pat, repl in SCRUB:
        text = pat.sub(repl, text)
    return text


# --- failure signature -----------------------------------------------------
#
# Deterministic, derived from the log -- never from the AI's prose, which wobbles
# run to run and would produce a "changed" comment every night.

ERR_RE = re.compile(
    r"^.*?(?:\berror\b|undefined reference|Assertion .* failed|"
    r"Segmentation fault|No rule to make target|identity gate).*$",
    re.M | re.I,
)
NORM = [
    # Drop the directory prefix entirely, not down to a leading "/": the same
    # error under /home/runner/... and a relative path must hash identically,
    # or every run reports a "signature changed" and the bot spams.
    (re.compile(r"[\w./+-]*/"), ""),
    (re.compile(r":\d+(:\d+)?\b"), ":N"),
    (re.compile(r"0x[0-9a-fA-F]+"), "0xADDR"),
    (re.compile(r"\b\d+\b"), "N"),
]


def signature(log):
    """Stable hash of what broke. Normalising paths and line numbers means an
    upstream whitespace shift does not notify, but a changed symbol does."""
    lines = ERR_RE.findall(log or "")[:3]
    if not lines:
        return "unknown"
    text = "\n".join(lines).lower()
    for pat, repl in NORM:
        text = pat.sub(repl, text)
    return hashlib.sha256(text.encode()).hexdigest()[:12]


# --- body ------------------------------------------------------------------


def parse_fp(body):
    m = FP_RE.search(body or "")
    return m.group(1) if m else None


def parse_state(body):
    m = STATE_RE.search(body or "")
    if not m:
        return {}
    try:
        return json.loads(m.group(1))
    except json.JSONDecodeError:
        return {}


def render_body(eid, state, ai, failing_rows, run_url, now):
    # The log never goes in the state marker. An HTML comment is invisible in
    # rendered markdown but fully readable via the API and the raw view, so an
    # unscrubbed log there is a silent leak into a PUBLIC tracker.
    log = state.get("log", "")
    marker = {k: v for k, v in state.items() if k != "log"}
    lines = [
        f"<!-- ci-fp: v1|{eid} -->",
        f"<!-- ci-state: {json.dumps(marker, sort_keys=True)} -->",
        "",
        f"**`{eid}`** is failing in the nightly — "
        f"{state.get('streak', 1)} night(s) running, since {state.get('first', now)}.",
        "",
    ]

    if ai:
        lines += [
            f"### {ai.get('severity', '?')} · {ai.get('category', '?')} · "
            f"affects {ai.get('blast', '?')}",
            "",
            f"- **Symptom** — {scrub(ai.get('symptom', ''))}",
            f"- **Likely cause** — {scrub(ai.get('cause', ''))}",
            f"- **Next** — {scrub(ai.get('next', ''))}",
            "",
        ]

    # "wolfSSL" not "wolfSSL ref": the column answers "which wolfSSL broke it",
    # and master-vs-stable is the single most useful fact here -- a stable-only
    # break is a released bug, a master-only one is upstream drift.
    lines += [
        "### What failed",
        "",
        "| test | wolfSSL | error |",
        "|---|---|---|",
    ]
    for row in failing_rows:
        test = f"`{row['test']}`"
        if row.get("url"):
            test = f"[{test}]({row['url']})"
        detail = (row.get("detail") or "").replace("|", "\\|")[:120] or "—"
        lines.append(f"| {test} | `{row['ref']}` | {detail} |")
    lines.append("")

    if ai and ai.get("upstream"):
        lines += [
            "> Fails only against wolfSSL `master` and passes on stable — this "
            "is more likely a wolfSSL regression than an example bug. Consider "
            "filing against wolfSSL/wolfssl.",
            "",
        ]

    if state.get("log"):
        lines += [
            "<details><summary>log tail</summary>",
            "",
            "```",
            scrub(state["log"]),
            "```",
            "",
            "</details>",
            "",
        ]

    lines += [
        "---",
        f"*Filed automatically by [ci-triage]({run_url}). Body updated {now} UTC.*",
        f"*Add the `{LABEL_MUTE}` label to stop all updates. Closing as "
        f"**not planned** prevents reopening.*",
    ]
    return "\n".join(lines)


# --- state machine ---------------------------------------------------------

PASS = "pass"
REAL = "real"
FLAKE_ONLY = "flake_only"
ABSENT = "absent"
UNCONFIRMED = "unconfirmed"


def decide(observation, issue, close_allowed=True):
    """Return (action, comment_reason).

    action: open | comment | update | close | reopen | noop
    """
    labels = {l["name"] if isinstance(l, dict) else l for l in (issue or {}).get("labels", [])}

    if issue and LABEL_MUTE in labels:
        return "noop", "muted"
    if issue and issue.get("state_reason") == "not_planned":
        return "noop", "wontfix"

    if observation == ABSENT:
        # Absence of evidence is not recovery. Only an observed pass closes.
        return "noop", "absent from this run"

    if observation == PASS:
        if issue and issue.get("state") == "open":
            if not close_allowed:
                return "update", "circuit breaker: refusing to close"
            if parse_state(issue.get("body", "")).get("sticky"):
                return "update", "sticky (flaky): human closes"
            return "close", "passing again"
        return "noop", "passing"

    if observation == FLAKE_ONLY:
        # A flake is a failure a retry hid, not a pass. It used to fall in with
        # PASS above, so a dir that flaked every night closed its own issue every
        # night and left no trace -- which is how the dtls resume flake stayed
        # invisible until two runs were diffed by hand. Only a clean night closes.
        if not issue:
            return "open", "flaked and recovered on retry"
        if issue.get("state") == "closed":
            return "reopen", "flaked again"
        return "update", "flaked and recovered on retry"

    # REAL or UNCONFIRMED
    if not issue:
        return "open", "first failure"
    if issue.get("state") == "closed":
        return "reopen", "failing again"
    return "update", "still failing"


def render_comment(state, ai, observation, reason, run_url, now):
    """One comment per night, so the thread carries the history.

    wolfHSM's nightly posts a bare "Still failing as of <url>" -- 15 of them on
    one issue, none of which say what happened. Carry the verdict and the retry
    outcome, so the thread is readable rather than just long.
    """
    n = state.get("streak", 1)
    verdict = ("**Real** — retried once, failed again." if observation == REAL
               else "**Unconfirmed** — the retry did not complete, so this is "
                    "not double-checked.")
    lines = [
        f"**Night {n}** · [run]({run_url}) · {now}",
        "",
        verdict,
    ]
    if reason:
        lines += ["", f"Changed since last night: **{reason}**."]
    if ai:
        lines += ["", f"`{ai.get('severity', '?')}` · {scrub(ai.get('symptom', ''))}"]
        if ai.get("next"):
            lines.append(f"_next:_ {scrub(ai.get('next', ''))}")
    refs = state.get("failing") or []
    if refs:
        lines += ["", f"Failing on: {', '.join(f'`{r}`' for r in refs)}"]
    return "\n".join(lines)


def material_change(old_state, new_state):
    """Comment only on a material change. Body edits are silent; comments notify,
    and a comment every night on a long-standing break gets the bot muted."""
    if not old_state:
        return "first failure"
    if old_state.get("sig") != new_state.get("sig"):
        return "failure signature changed"
    new_refs = set(new_state.get("failing", [])) - set(old_state.get("failing", []))
    if new_refs:
        return f"now also failing on {', '.join(sorted(new_refs))}"
    if not old_state.get("flaky_noted") and new_state.get("sticky"):
        return "flapping: 3rd open in 14 days"
    return None
