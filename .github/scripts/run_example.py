#!/usr/bin/env python3
"""Build and run one example directory from .github/examples-manifest.yml."""

import argparse
import json
import os
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MANIFEST = REPO / ".github" / "examples-manifest.yml"
NETNS_SENTINEL = "WOLFEX_IN_NETNS"
LISTEN = "0A"


def load_manifest():
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import manifest as mf

    return mf, mf.load(MANIFEST)


# --- wolfSSL identity gate -------------------------------------------------
#
# 5 host-tier Makefiles link -lwolfssl with no -I/-L and cannot be pointed at a
# prefix, so a path assertion is meaningless. Assert instead that exactly one
# libwolfssl is visible to the linker and that it is the one this job built.


def visible_libwolfssl():
    out = subprocess.run(
        ["ldconfig", "-p"], capture_output=True, text=True
    ).stdout
    paths = set()
    for line in out.splitlines():
        if "libwolfssl" in line and "=>" in line:
            paths.add(os.path.realpath(line.split("=>")[-1].strip()))
    for extra in ("/usr/local/lib", "/usr/lib", "/usr/lib64"):
        d = Path(extra)
        if d.is_dir():
            for p in d.glob("libwolfssl.so*"):
                paths.add(os.path.realpath(p))
    return {p for p in paths if Path(p).exists()}


def sha256(path):
    out = subprocess.run(
        ["sha256sum", str(path)], capture_output=True, text=True, check=True
    ).stdout
    return out.split()[0]


def assert_identity(expect_sha):
    found = visible_libwolfssl()
    if not found:
        sys.exit("identity gate: no libwolfssl visible to the linker at all")
    shas = {p: sha256(p) for p in sorted(found)}
    bad = {p: s for p, s in shas.items() if s != expect_sha}
    if bad:
        for p, s in bad.items():
            print(f"  {p}\n    sha256 {s}", file=sys.stderr)
        print(f"  expected sha256 {expect_sha}", file=sys.stderr)
        sys.exit(
            "identity gate: a libwolfssl that this job did not build is visible.\n"
            "A green run here would have tested the wrong library."
        )
    print(f"identity gate: ok ({len(shas)} lib(s), all sha256 {expect_sha[:12]})")


def assert_binary_links_ours(binary, expect_sha):
    out = subprocess.run(
        ["ldd", str(binary)], capture_output=True, text=True
    ).stdout
    for line in out.splitlines():
        if "libwolfssl" in line and "=>" in line:
            resolved = os.path.realpath(line.split("=>")[1].split("(")[0].strip())
            if sha256(resolved) != expect_sha:
                sys.exit(f"identity gate: {binary} links a foreign libwolfssl: {resolved}")
            return "dynamic"
    # Statically linked (or no wolfSSL): the job-level gate above already proved
    # only our libwolfssl exists on this box, so it can only have come from ours.
    return "static-or-none"


# --- network namespace -----------------------------------------------------
#
# tls/server-tls.c has no SO_REUSEADDR and is the active closer, so port 11111
# sits in TIME_WAIT ~60s. Serializing pairs is the WORST case (back-to-back
# binds). A private netns per run gives each pair its own port space.


USERNS = ["unshare", "--user", "--map-root-user", "--net"]
SUDONS = ["sudo", "-n", "-E", "unshare", "--net"]


def _works(argv):
    return subprocess.run(argv + ["--", "true"], capture_output=True).returncode == 0


def netns_prefix():
    """Pick a way to get a private netns with a usable loopback, or None.

    Probe the EXACT invocation we intend to run. `unshare --user --net true`
    succeeds without a uid mapping and proves nothing: --map-root-user then
    fails writing /proc/self/uid_map, which is the capability `ip link set lo up`
    actually needs.
    """
    if _works(USERNS):
        return USERNS
    # ubuntu 24.04 ships kernel.apparmor_restrict_unprivileged_userns=1
    subprocess.run(
        ["sudo", "-n", "sysctl", "-w", "kernel.apparmor_restrict_unprivileged_userns=0"],
        capture_output=True,
    )
    if _works(USERNS):
        return USERNS
    # Root netns: no userns needed. Costs root-owned artifacts, which is fine on
    # an ephemeral runner where the build already happened as the normal user.
    if _works(SUDONS):
        return SUDONS
    return None


def describe_net():
    """One line of ground truth about the network we ended up in."""
    import socket
    ns = os.readlink("/proc/self/ns/net") if os.path.exists("/proc/self/ns/net") else "?"
    v6 = "no"
    try:
        sk = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sk.bind(("::1", 0))
        sk.close()
        v6 = "yes"
    except OSError as e:
        v6 = f"no ({e.errno})"
    print(f"net: ns={ns} ipv6-loopback={v6}", flush=True)


def ensure_netns():
    """Re-exec self inside a private netns. Idempotent via sentinel."""
    if os.environ.get(NETNS_SENTINEL):
        return
    prefix = netns_prefix()
    if prefix is None:
        sys.exit(
            "no way to obtain a private network namespace: unprivileged userns is\n"
            "blocked (ubuntu 24.04 apparmor_restrict_unprivileged_userns), the\n"
            "sysctl did not take, and passwordless sudo is unavailable.\n"
            "Re-run with --no-netns to accept TIME_WAIT flakes on port 11111."
        )
    print(f"netns via: {' '.join(prefix)}", flush=True)
    env = dict(os.environ, **{NETNS_SENTINEL: "1"})
    argv = [sys.executable, os.path.abspath(__file__)] + sys.argv[1:]
    # A fresh netns can come up with ipv6 disabled, which makes ::1 unreachable
    # and dtls/client-dtls-ipv6 fail wolfSSL_connect with -308. Enable it and let
    # the kernel assign ::1 to lo. Both are namespaced, so this is local to us.
    inner = (
        "ip link set lo up && "
        "sysctl -qw net.ipv6.conf.lo.disable_ipv6=0 2>/dev/null; "
        "exec " + " ".join(shlex.quote(a) for a in argv)
    )
    os.execvpe(prefix[0], prefix + ["--", "sh", "-c", inner], env)


# --- readiness -------------------------------------------------------------
#
# Never probe with connect(): it burns the server's single accept(), so
# wolfSSL_accept fails and server-tls.c:161 does `goto exit` -- the server dies
# and every pair test fails. Poll /proc/net/tcp for a LISTEN socket instead.
# We are inside the server's netns (see ensure_netns), so this is its table.


def wait_listen(port, timeout=10.0, proto="tcp"):
    # DTLS is UDP: a bound udp socket never enters TCP's LISTEN state and never
    # appears in /proc/net/tcp at all, so a bound port is the only readiness
    # signal available.
    want = f"{port:04X}"
    paths = ("/proc/net/tcp", "/proc/net/tcp6")
    if proto == "udp":
        paths = ("/proc/net/udp", "/proc/net/udp6")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for path in paths:
            try:
                with open(path) as fh:
                    next(fh, None)
                    for line in fh:
                        col = line.split()
                        if len(col) > 3:
                            if proto == "tcp" and col[3] != LISTEN:
                                continue
                            if col[1].rsplit(":", 1)[-1].upper() == want:
                                return True
            except FileNotFoundError:
                pass
        time.sleep(0.05)
    return False


def reap(proc):
    if proc.poll() is None:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass


# --- runners ---------------------------------------------------------------


# --- output assertions -----------------------------------------------------
#
# ~30 examples are wrapped in a feature #ifdef whose #else is a stub main() that
# prints "requires --enable-foo" and returns 0. Checking only the exit code marks
# those a pass, so a profile missing a flag silently tests nothing.

STUB_MARKERS = (
    "requires --enable",
    "please define",
    "not compiled in",
    "must build wolfssl using",
    "must build wolfssl with",
    "example requires",
    "please configure wolfssl with",
    "please build wolfssl with",
    "pk not compiled in",
)


def stub_output(out):
    """Return the marker a rc==0 run printed, if it never really ran."""
    for line in (out or "").splitlines():
        low = line.lower()
        for m in STUB_MARKERS:
            if m in low:
                return line.strip()
    return None


def check_output(out, step):
    """(ok, detail) for a step that already exited 0."""
    marker = stub_output(out)
    if marker:
        return False, f"built a stub, not the example: {marker!r}"
    want = step.get("expect") if isinstance(step, dict) else None
    if want and want not in (out or ""):
        return False, f"expected {want!r} in output"
    return True, ""


def run_exec(spec, cwd, env, timeout):
    argv = spec if isinstance(spec, list) else shlex.split(spec)
    try:
        p = subprocess.Popen(
            argv,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
    except FileNotFoundError:
        return False, f"binary not found: {argv[0]}", ""
    try:
        out, _ = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        # Keep whatever it printed before it wedged: "timeout" with no output
        # says nothing about how far it got.
        reap(p)
        out, _ = p.communicate()
        return False, f"timeout after {timeout}s", (out or "")[-2000:]
    if p.returncode != 0:
        return False, f"rc={p.returncode}", (out or "")[-2000:]
    return True, "", out or ""


def run_negative(spec, cwd, env, timeout, want):
    """A step given bad input that MUST be rejected: passing means a clean
    nonzero exit. rc==0 is a false green, a signal is a crash, a timeout is a
    hang -- all three are failures, not rejections."""
    argv = spec if isinstance(spec, list) else shlex.split(spec)
    try:
        p = subprocess.Popen(
            argv,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
    except FileNotFoundError:
        return False, f"binary not found: {argv[0]}", ""
    try:
        out, _ = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        reap(p)
        out, _ = p.communicate()
        return False, f"hung on bad input (timeout {timeout}s)", (out or "")[-2000:]
    out = out or ""
    if p.returncode == 0:
        return False, "accepted bad input (rc=0), expected rejection", out[-2000:]
    if p.returncode < 0:
        return False, f"crashed on bad input (signal {-p.returncode})", out[-2000:]
    if want and want not in out:
        return False, f"rejected but missing error {want!r}", out[-2000:]
    return True, "", ""


def start_bg(argv, cwd, env, stdin_data=None):
    """Start a background peer, line buffered so its output survives a kill.

    A pipe makes stdout fully buffered, and SIGKILL then throws the buffer away
    -- which is why every failing pair used to show an empty server side.
    """
    if shutil.which("stdbuf"):
        argv = ["stdbuf", "-oL", "-eL"] + list(argv)
    proc = subprocess.Popen(
        argv,
        cwd=cwd,
        env=env,
        stdin=subprocess.PIPE if stdin_data is not None else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )
    if stdin_data is not None:
        try:
            proc.stdin.write(stdin_data)
            proc.stdin.flush()
        except (BrokenPipeError, OSError):
            pass
        # left open on purpose: EOF here makes openssl s_server drop the peer
        # before our client has read the reply
    return proc


def await_ready(proc, port, proto, ready_delay, ready_timeout, what="server"):
    """Wait for a background peer. Returns (ok, headline, detail)."""
    if port in (None, "none", 0):
        # No socket to poll: give it a moment to open its transport. btle talks
        # over FIFOs in /tmp and custom-io over files, so waiting for a LISTEN
        # that never comes would fail every time.
        time.sleep(ready_delay)
        return True, "", ""
    if wait_listen(port, ready_timeout, proto):
        return True, "", ""
    diag = [f"{what} never bound {proto} :{port}"]
    if proc.poll() is not None:
        diag.append(f"{what} already exited rc={proc.returncode}")
        try:
            diag.append((proc.stdout.read() or "")[-600:])
        except Exception:
            pass
    else:
        diag.append(f"{what} still running but not listening")
    for f in ("/proc/net/tcp", "/proc/net/tcp6", "/proc/net/udp", "/proc/net/udp6"):
        try:
            hits = [
                l.split()[1:4]
                for l in open(f).read().splitlines()[1:]
                if l.split()[1].rsplit(":", 1)[-1].upper() == f"{port:04X}"
            ]
            if hits:
                diag.append(f"{f}: {hits}")
        except (FileNotFoundError, IndexError):
            pass
    return False, diag[0], "\n".join(diag[1:])


def run_pair(spec, cwd, env):
    port = spec.get("port", 11111)
    proto = spec.get("proto", "tcp")
    timeout = spec.get("timeout", 30)
    server_exit = spec.get("server_exit", "clean")

    srv = start_bg(spec["server"], cwd, env, spec.get("server_stdin"))
    try:
        ok, head, detail = await_ready(
            srv, port, proto, spec.get("ready_delay", 2), spec.get("ready_timeout", 10)
        )
        if not ok:
            return False, head, detail

        try:
            cli = subprocess.run(
                spec["client"],
                cwd=cwd,
                env=env,
                input=spec.get("stdin", ""),
                capture_output=True,
                text=True,
                timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            return False, f"client timeout after {timeout}s", ""
        if cli.returncode != 0:
            # The reason is usually on the server's side; a client rc with no
            # server output is close to undiagnosable.
            reap(srv)
            try:
                srv_out = (srv.stdout.read() or "")[-1200:]
            except Exception:
                srv_out = "(server output unavailable)"
            return False, f"client rc={cli.returncode}", (
                "--- client ---\n" + (cli.stdout + cli.stderr)[-1200:]
                + "\n--- server ---\n" + srv_out
            )

        if server_exit == "clean":
            try:
                rc = srv.wait(timeout=10)
            except subprocess.TimeoutExpired:
                return False, "server did not exit after client completed", ""
            if rc != 0:
                # Client rc==0 is necessary but not sufficient: the client can
                # finish its exchange while the server dies in cleanup.
                return False, f"client ok but server rc={rc}", (srv.stdout.read() or "")[-2000:]
        else:
            os.killpg(srv.pid, signal.SIGTERM)
            try:
                srv.wait(timeout=5)
            except subprocess.TimeoutExpired:
                return False, "server ignored SIGTERM", ""
        # the client's output, so expect: has something to match on
        return True, "", (cli.stdout or "") + (cli.stderr or "")
    finally:
        reap(srv)


def run_procs(spec, cwd, env):
    """N background peers, then one foreground driver.

    `pair:` is the 2-process case and stays as-is. This is for the chains that
    genuinely need more: ocsp/stapling is responder + server + client, and
    pq/pqc_proxy is origin + proxy + client. Writing a check.sh per directory
    instead would duplicate this orchestration once per example.

        procs:
          background:
            - argv: [./legacy-server]
              port: 11111
            - argv: [./pq-proxy]
              port: 11112
          client: [./pq-client, 127.0.0.1]
    """
    timeout = spec.get("timeout", 30)
    procs = []
    try:
        for bg in spec.get("background", []):
            p = start_bg(bg["argv"], cwd, env)
            procs.append(p)
            ok, head, detail = await_ready(
                p, bg.get("port"), bg.get("proto", "tcp"),
                bg.get("ready_delay", 2), bg.get("ready_timeout", 10),
                what=bg["argv"][0],
            )
            if not ok:
                return False, head, detail

        try:
            cli = subprocess.run(
                spec["client"], cwd=cwd, env=env, input=spec.get("stdin", ""),
                capture_output=True, text=True, timeout=timeout,
            )
        except subprocess.TimeoutExpired:
            return False, f"client timeout after {timeout}s", bg_output(procs)
        if cli.returncode != 0:
            return False, f"client rc={cli.returncode}", (
                "--- client ---\n" + (cli.stdout + cli.stderr)[-1200:]
                + bg_output(procs)
            )
        # Return the driver's output, not "": otherwise `expect:` has nothing to
        # match and every asserted step fails. run_exec already does this; run_pair
        # does not, which is why no pair carries an expect: today.
        return True, "", (cli.stdout or "") + (cli.stderr or "")
    finally:
        for p in procs:
            reap(p)


def bg_output(procs):
    """Whatever the background peers said. A client rc with no peer output is
    close to undiagnosable, which is why every proc is reaped first."""
    out = []
    for p in procs:
        reap(p)
        try:
            out.append(f"\n--- {p.args[-1] if p.args else 'peer'} ---\n"
                       + (p.stdout.read() or "")[-800:])
        except Exception:
            out.append("\n--- peer output unavailable ---")
    return "".join(out)


def run_script(spec, cwd, env, timeout):
    argv = spec if isinstance(spec, list) else shlex.split(spec)
    try:
        p = subprocess.run(
            argv, cwd=cwd, env=env, capture_output=True, text=True, timeout=timeout
        )
    except subprocess.TimeoutExpired:
        return False, f"timeout after {timeout}s", ""
    if p.returncode != 0:
        return False, f"rc={p.returncode}", (p.stdout + p.stderr)[-2000:]
    # hand the output back on success too, or expect: has nothing to match on
    return True, "", (p.stdout or "") + (p.stderr or "")


def build(entry, cwd):
    system = entry.get("build", "make")
    if system == "none":
        return True, "", ""
    # A list is a literal command, for dirs whose README documents a bare gcc
    # line and ships no Makefile (e.g. pq/ml_kem).
    if isinstance(system, list):
        argv = system
    elif system == "make":
        argv = ["make", "-j", str(os.cpu_count() or 2)]
    else:
        return False, f"build system '{system}' not handled by this harness", ""
    p = subprocess.run(argv, cwd=cwd, capture_output=True, text=True)
    if p.returncode != 0:
        return False, f"{argv[0]} rc={p.returncode}", (p.stdout + p.stderr)[-4000:]
    return True, "", ""


BINARY_STUB_MARKERS = (
    "requires --enable",
    "Must build wolfSSL using",
    "Example requires",
    "Please configure wolfSSL with",
    "Please build wolfssl with",
    "PK not compiled in",
)


def stub_binaries(cwd):
    """Binaries that compiled a stub main(), without running anything.

    A stub is the #else of a feature #ifdef, so its "requires --enable-foo"
    string is only present when the feature was NOT compiled in. Finding the
    marker inside the binary therefore proves the stub was built -- which is how
    build-only examples get checked at all, since they never execute.
    """
    bad = []
    for f in sorted(Path(cwd).iterdir()):
        if not f.is_file() or not os.access(f, os.X_OK) or f.suffix:
            continue
        try:
            blob = f.read_bytes()
        except OSError:
            continue
        # ELF on the runners; Mach-O too so this is verifiable on a dev mac
        if not blob.startswith((b"\x7fELF", b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe")):
            continue
        for m in BINARY_STUB_MARKERS:
            if m.encode() in blob:
                bad.append(f"{f.name}: {m!r}")
                break
    return bad


def do_fetch(entries):
    """Run every fetch: step. Returns a process exit code."""
    rc = 0
    for e in entries:
        for step in e.get("fetch") or []:
            cwd = REPO / e["path"]
            print(f"fetch {e['id']}: {' '.join(step)}", flush=True)
            ok, detail, log = run_script(step, cwd, dict(os.environ), 600)
            # print on success too: a fetch that "works" but produces nothing is
            # exactly the case worth seeing
            if log:
                print(log, flush=True)
            if not ok:
                print(f"FAIL fetch {e['id']}: {detail}", flush=True)
                rc = 1
    return rc


def attempt_step(step, cwd, env):
    """Run one step once, output check included. Returns (label, ok, detail, log);
    label is None for an unrecognized step kind the caller should skip."""
    if "exec" in step:
        label = " ".join(step["exec"])
        ok, detail, log = run_exec(step["exec"], cwd, env, step.get("timeout", 60))
    elif "pair" in step:
        label = f"{step['pair']['server'][0]} + {step['pair']['client'][0]}"
        ok, detail, log = run_pair(step["pair"], cwd, env)
    elif "procs" in step:
        bg = [b["argv"][0] for b in step["procs"].get("background", [])]
        label = " + ".join(bg + [step["procs"]["client"][0]])
        ok, detail, log = run_procs(step["procs"], cwd, env)
    elif "script" in step:
        label = " ".join(step["script"])
        ok, detail, log = run_script(step["script"], cwd, env, step.get("timeout", 300))
    else:
        return None, True, "", ""
    if ok:
        ok, odetail = check_output(log, step)
        if not ok:
            detail, log = odetail, (log or "")[-2000:]
        else:
            log = ""
    return label, ok, detail, log


def run_entry(entry, expect_sha, results):
    eid = entry["id"]
    cwd = REPO / entry["path"]
    env = dict(os.environ)
    env.update({k: os.path.expandvars(v) for k, v in (entry.get("env") or {}).items()})

    if entry.get("mode") == "skip":
        results.append({"id": eid, "status": "skip", "detail": entry.get("reason", "")})
        return True

    # Before build, not after: setup is where an example's third-party tree gets
    # fetched (RIOT, picoTCP), and the build needs it to already be there.
    for step in entry.get("setup") or []:
        ok, detail, log = run_script(step, cwd, env, 300)
        if not ok:
            results.append({"id": eid, "status": "fail", "stage": "setup", "detail": detail, "log": log})
            return False

    ok, detail, log = build(entry, cwd)
    if not ok:
        results.append({"id": eid, "status": "fail", "stage": "build", "detail": detail, "log": log})
        return False

    stubs = stub_binaries(cwd)
    if stubs:
        results.append({
            "id": eid, "status": "fail", "stage": "build",
            "detail": "built a stub, not the example: " + "; ".join(stubs),
            "log": "",
        })
        return False

    if entry.get("mode") == "build-only":
        results.append({"id": eid, "status": "pass", "stage": "build-only"})
        return True

    for target in entry.get("targets") or []:
        binary = cwd / target
        if binary.exists():
            assert_binary_links_ours(binary, expect_sha)

    all_ok = True
    for step in entry.get("run") or []:
        if "exec" in step and step.get("must_fail"):
            label = " ".join(step["exec"]) + " (must reject)"
            ok, detail, log = run_negative(
                step["exec"], cwd, env, step.get("timeout", 60), step.get("expect")
            )
            results.append(
                {
                    "id": eid,
                    "target": label,
                    "status": "pass" if ok else "fail",
                    "stage": "run",
                    "detail": detail,
                    "log": log,
                }
            )
            all_ok = all_ok and ok
            continue
        # repeat: N re-runs a step to surface per-run randomness (keygen, signing,
        # PQ KEM); one silent pass in N hides an intermittent failure otherwise.
        n = max(1, int(step.get("repeat", 1)))
        label, ok, detail, log = attempt_step(step, cwd, env)
        if label is None:
            continue
        run_no = 1
        while ok and run_no < n:
            run_no += 1
            _, ok, detail, log = attempt_step(step, cwd, env)
        if n > 1:
            label = f"{label} (x{n})"
            detail = f"{n} runs ok" if ok else f"flaked on run {run_no}/{n}: {detail}"

        xfail = step.get("expect_fail")
        if xfail and not ok:
            status, detail = "xfail", f"known: {xfail}"
            ok = True
        elif xfail and ok:
            # Passing means the upstream fix landed; fail so the marker gets removed
            # rather than silently masking a future regression.
            status, detail = "fail", f"passed unexpectedly, drop expect_fail: {xfail}"
            ok = False
        else:
            status = "pass" if ok else "fail"

        results.append(
            {
                "id": eid,
                "target": label,
                "status": status,
                "stage": "run",
                "detail": detail,
                "log": log,
            }
        )
        all_ok = all_ok and ok
    return all_ok


def write_summary(results):
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    lines = ["| example | target | result | detail |", "|---|---|---|---|"]
    icon = {"pass": ":white_check_mark:", "fail": ":x:", "skip": ":fast_forward:",
            "xfail": ":warning:"}
    for r in results:
        lines.append(
            f"| `{r['id']}` | `{r.get('target', r.get('stage', ''))}` "
            f"| {icon.get(r['status'], r['status'])} | {r.get('detail', '')} |"
        )
    with open(path, "a") as fh:
        fh.write("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", help="run a single example by id")
    ap.add_argument("--tier", help="run every example in this tier")
    ap.add_argument("--expect-sha", help="sha256 of the libwolfssl this job built")
    ap.add_argument("--results", help="write per-example JSON results here")
    ap.add_argument(
        "--wolfssl-ref",
        default="",
        help="wolfSSL ref this job built against; stamped on every result row so "
        "triage can tell master from stable instead of collapsing them",
    )
    ap.add_argument("--no-netns", action="store_true")
    ap.add_argument(
        "--fetch",
        action="store_true",
        help="run only the fetch: steps, outside the netns, and exit. The run "
        "itself is sandboxed with no route off loopback, so anything that has to "
        "come off the internet (a third-party source tree) has to arrive first.",
    )
    args = ap.parse_args()

    mf, data = load_manifest()
    entries = [e for e in data["examples"] if e.get("mode") != "skip"]
    if args.only:
        entries = [e for e in entries if e["id"] == args.only]
    if args.tier:
        entries = [e for e in entries if e.get("tier", "host") == args.tier]
    if not entries:
        sys.exit(f"no examples matched (--only={args.only} --tier={args.tier})")

    # Deliberately before ensure_netns(): that call execs the whole process into a
    # namespace with only loopback, so a fetch after it can never reach a remote.
    if args.fetch:
        sys.exit(do_fetch(entries))

    if not args.no_netns:
        ensure_netns()
    describe_net()

    expect_sha = args.expect_sha
    if expect_sha:
        try:
            assert_identity(expect_sha)
        except BaseException as e:
            # Exiting here with no results file makes triage read the job as
            # never-ran rather than failed, so leave it a row first.
            if args.results:
                Path(args.results).write_text(json.dumps([{
                    "id": args.only or "(all)",
                    "target": "identity-gate",
                    "status": "fail",
                    "detail": str(e),
                    "ref": args.wolfssl_ref,
                }], indent=2))
            raise

    results = []
    ok = True
    for entry in entries:
        ok = run_entry(entry, expect_sha, results) and ok

    for r in results:
        r["ref"] = args.wolfssl_ref

    write_summary(results)
    if args.results:
        Path(args.results).write_text(json.dumps(results, indent=2))

    for r in results:
        if r["status"] == "fail":
            print(f"FAIL {r['id']} {r.get('target', '')}: {r.get('detail', '')}", file=sys.stderr)
            if r.get("log"):
                print(r["log"], file=sys.stderr)
    print(
        f"\n{sum(1 for r in results if r['status'] == 'pass')} passed, "
        f"{sum(1 for r in results if r['status'] == 'fail')} failed, "
        f"{sum(1 for r in results if r['status'] == 'skip')} skipped, "
        f"{sum(1 for r in results if r['status'] == 'xfail')} known-fail"
    )
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
