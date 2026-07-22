#!/usr/bin/env python3
"""Read examples-manifest.yml: emit the CI matrix, or gate coverage."""

import argparse
import collections
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MANIFEST = REPO / ".github" / "examples-manifest.yml"

# Dirs that are not examples and never will be.
NOT_EXAMPLES = {".git", ".github", "certs"}

# A dir is "buildable source" -- and therefore must be accounted for -- if it
# holds any of these. Gating on Makefile alone missed SGX (own scripts), Android
# (gradle), ESP32 (idf/cmake) and the README-gcc-one-liner dirs, which is exactly
# the "someone added an example and forgot to wire it up" case the gate exists for.
BUILD_MARKERS = ("Makefile", "CMakeLists.txt", "build.gradle", "build.gradle.kts")

# Project files are named after their project, so they match by suffix, not name.
# A .sln is a container for these, not a build unit itself.
BUILD_MARKER_SUFFIXES = (".csproj", ".vcxproj")

# Not just .c: an example written in another language is still an example, and a
# gate that cannot see it cannot force a decision about it. .ino in particular
# looked covered -- arduino.yml compiles the examples inside the *installed*
# Arduino wolfSSL library, not this repo's Arduino/sketches/*.
SOURCE_SUFFIXES = (".c", ".ino", ".java", ".cpp", ".cs")

# Dirs that carry a build marker but are not independently buildable units:
# IDE projects, generated tool config, and subproject components.
NOISE = (
    "/VisualGDB",       # IDE projects mirroring a parent Makefile's targets
    "/.config/",        # generated CCS/XDC config under tirtos_ccs_examples
    "/components/",     # ESP-IDF components of a parent project
    "/wolfssl_library/", # the Arduino template's bundled library, not a unit
    "/src/com/",        # java package tree of an android project declared above it
)


def is_noise(d):
    s = f"/{d}/"
    return any(n in s for n in NOISE) or d.endswith("/main")


def load(path=MANIFEST):
    try:
        import yaml
    except ImportError:
        sys.exit("PyYAML required: pip install pyyaml")
    with open(path) as fh:
        data = yaml.safe_load(fh)
    validate(data)
    return data


def validate(data):
    profiles = data.get("profiles") or {}
    seen = set()
    for e in data.get("examples") or []:
        for key in ("id", "path"):
            if key not in e:
                sys.exit(f"manifest: entry missing '{key}': {e}")
        if e["id"] in seen:
            sys.exit(f"manifest: duplicate id '{e['id']}'")
        seen.add(e["id"])
        mode = e.get("mode", "run")
        if mode not in ("run", "check", "build-only", "skip"):
            sys.exit(f"manifest: {e['id']}: bad mode '{mode}'")
        if mode == "skip" and not e.get("reason"):
            sys.exit(f"manifest: {e['id']}: mode 'skip' requires a 'reason'")
        tier = e.get("tier", "host")
        if tier not in ("host", "emulated", "cross"):
            sys.exit(f"manifest: {e['id']}: bad tier '{tier}'")
        # A host example that only builds is an untested example, so say why.
        # Cross-tier entries are exempt: for a cross-compile target, building is
        # the test.
        if mode == "build-only" and tier == "host" and not e.get("reason"):
            sys.exit(
                f"manifest: {e['id']}: host 'build-only' requires a 'reason'.\n"
                "Add a run: recipe, or state why it cannot run in CI."
            )
        if mode != "skip":
            if not e.get("profile"):
                sys.exit(f"manifest: {e['id']}: needs a 'profile'")
            if e["profile"] not in profiles:
                sys.exit(f"manifest: {e['id']}: unknown profile '{e['profile']}'")
            if not (REPO / e["path"]).is_dir():
                sys.exit(f"manifest: {e['id']}: path does not exist: {e['path']}")


def job_built_paths():
    """Paths some cross-tier workflow actually builds.

    Read from the workflows rather than declared in the manifest: a manifest
    that states its own coverage would just agree with itself.
    """
    import yaml
    wf_dir = REPO / ".github/workflows"
    # one workflow per target, so scan them all rather than a single file
    paths = set()
    jobs_seen = set()
    for f in sorted(wf_dir.glob("*.yml")):
        try:
            wf = yaml.safe_load(f.read_text())
        except Exception:
            continue
        if not isinstance(wf, dict):
            continue
        for name, job in (wf.get("jobs") or {}).items():
            if not isinstance(job, dict):
                continue
            jobs_seen.add(name)
            matrix = ((job.get("strategy") or {}).get("matrix") or {})
            for item in matrix.get("example") or []:
                if isinstance(item, str):
                    paths.add(item)
            for item in matrix.get("include") or []:
                if isinstance(item, dict) and isinstance(item.get("example"), str):
                    paths.add(item["example"])

    # arduino.yml runs compile-all-examples.sh over board_list.txt, so it covers
    # every sketch dir rather than naming them in a matrix
    if (REPO / ".github/workflows/arduino.yml").is_file():
        for d in (REPO / "Arduino/sketches").glob("*/"):
            if d.is_dir():
                paths.add("Arduino/sketches/" + d.name)
    # jobs that build one hardcoded dir rather than a matrix
    # csharp: the client is the peer in every matrix leg rather than an axis
    for job, path in (("puf", "puf"), ("rpi-pico", "RPi-Pico"),
                      ("fullstack", "fullstack/freertos-wolfip-wolfssl-https"),
                      ("java", "java/https-url"), ("rt1060", "RT1060"),
                      ("csharp", "CSharp/wolfSSL-TLS-pq-Client"),
                      ("psa", "psa"),
                      ("bsdkm", "kernel/bsdkm"),
                      ("cmake", "cmake")):
        if job in jobs_seen:
            paths.add(path)
    # the pico job builds the whole cmake tree, so its subdirs come with it
    pico = REPO / "RPi-Pico/CMakeLists.txt"
    if pico.is_file() and "RPi-Pico" in paths:
        for sub in re.findall(r"add_subdirectory\(([^)]+)\)", pico.read_text()):
            paths.add("RPi-Pico/" + sub.strip().strip('"').strip("'"))
    return paths


def emulated_jobs():
    emul = (REPO / ".github/workflows/emulated.yml").read_text()
    body = emul.split("\njobs:", 1)[-1]
    return set(re.findall(r"^  ([a-z0-9][a-z0-9-]*):$", body, re.M))


def validate_has_job(data):
    """A cross/emulated entry that is not skipped must be built by some job.

    Without this, `mode: build-only` on an entry no workflow touches reads as
    covered while nothing compiles it.
    """
    built = job_built_paths()
    emul = emulated_jobs()
    orphans = []
    for e in data.get("examples") or []:
        tier = e.get("tier", "host")
        if tier not in ("cross", "emulated") or e.get("mode", "run") == "skip":
            continue
        if tier == "emulated":
            # emulated ids are the job names (sgx-linux -> sgx)
            if not any(j == e["id"] or e["id"].startswith(j) for j in emul):
                orphans.append(e["id"])
            continue
        p = e["path"]
        if not any(p == b or p.startswith(b + "/") for b in built):
            orphans.append(e["id"])
    if orphans:
        sys.exit(
            "manifest: no job builds these, so 'build-only' is not true:\n  "
            + "\n  ".join(orphans)
            + "\n\nAdd a job, or set mode: skip with a reason."
        )


def buildable_dirs():
    """Topmost buildable units, relative to the repo root.

    A dir counts only if no ancestor already has a build marker -- otherwise
    tls/VisualGDB-tls/* (28 IDE subdirs under tls/Makefile) and ESP32/*/main
    (an ESP-IDF component of its parent project) would each demand an entry.
    """
    tracked = subprocess.run(
        ["git", "ls-files"], cwd=REPO, capture_output=True, text=True, check=True
    ).stdout.splitlines()

    marker_dirs, source_dirs = set(), set()
    for f in tracked:
        p = Path(f)
        if not p.parts or p.parts[0] in NOT_EXAMPLES:
            continue
        if p.name in BUILD_MARKERS or p.suffix in BUILD_MARKER_SUFFIXES:
            marker_dirs.add(str(p.parent))
        elif p.suffix in SOURCE_SUFFIXES and len(p.parts) > 1:
            source_dirs.add(str(p.parent))

    def has_buildable_ancestor(d):
        return any(str(a) in marker_dirs for a in Path(d).parents if str(a) != ".")

    # A dir with its own build marker is its own unit (signature/rsa_buffer has a
    # Makefile independent of signature/'s). A dir with only .c is a unit just
    # when nothing above it builds -- otherwise every ESP-IDF source subdir counts.
    units = {d for d in marker_dirs}
    units |= {d for d in source_dirs if not has_buildable_ancestor(d)}
    return {d for d in units if not is_noise(d)}


def cmd_check(data):
    validate_has_job(data)
    declared = {e["path"] for e in data["examples"]}
    actual = buildable_dirs()

    missing = sorted(actual - declared)
    stale = sorted(d for d in declared - actual if not (REPO / d).is_dir())

    if stale:
        print("Manifest entries whose path no longer exists:", file=sys.stderr)
        for d in stale:
            print(f"  {d}", file=sys.stderr)

    if missing:
        print(
            "\nThese dirs hold buildable source but are not in "
            ".github/examples-manifest.yml.\nAdd an entry, or add one with "
            "`mode: skip` and a `reason:`.\n",
            file=sys.stderr,
        )
        for d in missing:
            print(f"  {d}", file=sys.stderr)

    if missing or stale:
        sys.exit(1)
    print(f"coverage: ok ({len(declared)} entries cover {len(actual)} buildable dirs)")

    # The job count is one per entry per ref whatever the mode, so it never moves
    # when an example is promoted. Print the tally, or a reviewer cannot see it.
    modes = collections.Counter(e.get("mode", "run") for e in data["examples"])
    asserts = sum(
        1
        for e in data["examples"]
        for s in (e.get("run") or [])
        if isinstance(s, dict) and "expect" in s
    )
    xfails = sum(
        1
        for e in data["examples"]
        for s in (e.get("run") or [])
        if isinstance(s, dict) and "expect_fail" in s
    )
    print(
        f"tested: {modes['run']} run, {modes['check']} make-check, "
        f"{modes['build-only']} build-only, {modes['skip']} skip -- "
        f"{asserts + modes['check']} output assertions, {xfails} known-fail"
    )


def live_entries(data, tier):
    return [
        e
        for e in data["examples"]
        if e.get("mode") != "skip" and e.get("tier", "host") == tier
    ]


def entry_refs(entry, refs):
    ref = entry.get("wolfssl_ref")
    return [ref] if ref else refs


def cmd_matrix(data, refs, tier, shas=None):
    """One matrix entry per (example, ref) -- each example is its own job.

    A profile is a property of a directory, never an axis: pk/rsa is only ever
    'fastmath'. Never cross profiles x examples.

    `shas` pins each ref to a commit resolved once upstream, so every job shares
    one cache key instead of racing a moving branch.
    """
    pinned = dict(zip(refs, shas)) if shas else {}
    out = []
    for e in sorted(live_entries(data, tier), key=lambda x: x["id"]):
        p = data["profiles"][e["profile"]]
        # wolfssl_ref pins one example to a specific ref (e.g. refs/pull/N/head)
        # while a fix is in flight upstream. setup-wolfssl still resolves it to a
        # sha, so the cache key stays sha-based.
        for ref in entry_refs(e, refs):
            out.append(
                {
                    "id": e["id"],
                    "path": e["path"],
                    "profile": e["profile"],
                    # in the job title, so a green tile says whether it ran or
                    # only compiled instead of implying more than it proves
                    "mode_label": "Build" if e.get("mode") == "build-only" else "Run",
                    "wolfssl_ref": ref,
                    # refs/pull/N/head has slashes; anything used in a filename or
                    # artifact name needs the slug, not the ref.
                    "ref_slug": ref.replace("/", "-"),
                    "wolfssl_sha": pinned.get(ref, ref),
                    "flags": " ".join(p.get("flags", "").split()),
                    "cflags": p.get("cflags", ""),
                    "deps": " ".join(e.get("deps") or []),
                }
            )
    if len(out) > 256:
        sys.exit(
            f"matrix would be {len(out)} jobs; GitHub caps a matrix at 256.\n"
            "Reduce refs, or split this tier across workflows."
        )
    print(json.dumps(out))


def cmd_wolfssl_matrix(data, refs, tier, shas=None):
    """The distinct (profile, ref) builds this tier needs.

    Built once each and cached, so the per-example jobs restore rather than
    rebuild -- otherwise every example pays a full wolfSSL build.
    """
    pinned = dict(zip(refs, shas)) if shas else {}
    profiles = sorted({e["profile"] for e in live_entries(data, tier)})
    out = [
        {
            "profile": name,
            "wolfssl_ref": ref,
            "wolfssl_sha": pinned.get(ref, ref),
            "flags": " ".join(data["profiles"][name].get("flags", "").split()),
            "cflags": data["profiles"][name].get("cflags", ""),
        }
        for name in profiles
        for ref in refs
    ]
    # A pinned example needs its (profile, ref) seeded too, or its job pays a
    # full wolfSSL build on every run.
    for e in live_entries(data, tier):
        ref = e.get("wolfssl_ref")
        if ref and not any(
            o["profile"] == e["profile"] and o["wolfssl_ref"] == ref for o in out
        ):
            out.append(
                {
                    "profile": e["profile"],
                    "wolfssl_ref": ref,
                    "wolfssl_sha": pinned.get(ref, ref),
                    "flags": " ".join(
                        data["profiles"][e["profile"]].get("flags", "").split()
                    ),
                    "cflags": data["profiles"][e["profile"]].get("cflags", ""),
                }
            )
    print(json.dumps(out))


def cmd_skips(data):
    """Report every dir CI does not build, and why. Generated, never hand-kept."""
    skips = sorted(
        (e for e in data["examples"] if e.get("mode") == "skip"),
        key=lambda e: e["path"],
    )
    bo = sorted(
        (e for e in data["examples"] if e.get("mode") == "build-only"),
        key=lambda e: e["path"],
    )
    print(f"# Directories CI does not run\n")
    print(f"Generated by `manifest.py skips` from `.github/examples-manifest.yml`.\n")
    print(f"{len(skips)} not built at all; {len(bo)} built but not run.\n")
    print("## Not built\n")
    print("| directory | why |")
    print("|---|---|")
    for e in skips:
        print(f"| `{e['path']}` | {e['reason']} |")
    print("\n## Built, but not run\n")
    print("| directory | why not run |")
    print("|---|---|")
    for e in bo:
        print(f"| `{e['path']}` | {e.get('reason', 'no run recipe wired yet')} |")


def cmd_profiles(data):
    """Emit the distinct profiles a matrix needs to build, with their flags."""
    used = {e["profile"] for e in data["examples"] if e.get("mode") != "skip"}
    out = [
        {
            "profile": name,
            "flags": data["profiles"][name].get("flags", "").strip(),
            "cflags": data["profiles"][name].get("cflags", ""),
        }
        for name in sorted(used)
    ]
    print(json.dumps(out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "command", choices=["check", "matrix", "wolfssl-matrix", "profiles", "skips"]
    )
    ap.add_argument("--refs", default="master", help="comma-separated wolfSSL refs")
    ap.add_argument("--shas", default="", help="commit SHAs for --refs, same order")
    ap.add_argument("--tier", default="host", choices=["host", "emulated", "cross"])
    args = ap.parse_args()

    refs = args.refs.split(",")
    shas = args.shas.split(",") if args.shas else None
    if shas and len(shas) != len(refs):
        sys.exit(f"--shas has {len(shas)} entries but --refs has {len(refs)}")

    data = load()
    if args.command == "check":
        cmd_check(data)
    elif args.command == "matrix":
        cmd_matrix(data, refs, args.tier, shas)
    elif args.command == "wolfssl-matrix":
        cmd_wolfssl_matrix(data, refs, args.tier, shas)
    elif args.command == "skips":
        cmd_skips(data)
    else:
        cmd_profiles(data)


if __name__ == "__main__":
    main()
