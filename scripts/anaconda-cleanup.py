#!/usr/bin/env python3
"""Clean an Anaconda label by deleting every package version except the latest.

Package discovery uses the label's repodata.json files. Deletion uses the
anaconda-client CLI. The script is a dry run unless --execute is supplied.
"""

import argparse
import json
import subprocess as sp
import sys
from collections import defaultdict
from dataclasses import dataclass
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

try:
    # Prefer Conda's own version semantics when Conda is installed.
    from conda.models.version import VersionOrder
except ImportError:
    from packaging.version import Version as VersionOrder


DEFAULT_OWNER = "potassco"
DEFAULT_SUBDIRS = (
    "noarch",
    "linux-64",
    "linux-aarch64",
    "linux-ppc64le",
    "osx-64",
    "osx-arm64",
    "win-64",
)


@dataclass(frozen=True)
class Package:
    name: str
    version: str
    build: str
    build_number: int
    subdir: str
    filename: str

    def spec(self, owner: str) -> str:
        # This is the same fully qualified form accepted by `anaconda show`.
        return f"{owner}/{self.name}/{self.version}/{self.subdir}/{self.filename}"


def load_json(url: str):
    request = Request(url, headers={"User-Agent": "anaconda-label-cleanup/1.0"})
    with urlopen(request, timeout=60) as response:
        return json.load(response)


def get_packages(owner: str, label: str, subdirs):
    """Read and combine repodata.json from every requested subdir."""
    packages = []
    base_url = f"https://conda.anaconda.org/{owner}/label/{label}"

    for subdir in subdirs:
        url = f"{base_url}/{subdir}/repodata.json"
        try:
            repodata = load_json(url)
        except HTTPError as exc:
            if exc.code == 404:
                print(f"Skipping unavailable subdir {subdir}", file=sys.stderr)
                continue
            raise RuntimeError(f"Could not read {url}: HTTP {exc.code}") from exc
        except (URLError, TimeoutError, json.JSONDecodeError) as exc:
            raise RuntimeError(f"Could not read {url}: {exc}") from exc

        count = 0
        # Modern .conda artifacts and older .tar.bz2 artifacts are stored in
        # separate maps. Read both so the inventory is complete.
        for section in ("packages", "packages.conda"):
            for filename, metadata in repodata.get(section, {}).items():
                packages.append(
                    Package(
                        name=metadata["name"],
                        version=metadata["version"],
                        build=metadata.get("build", ""),
                        build_number=metadata.get("build_number", 0),
                        subdir=metadata.get("subdir", subdir),
                        filename=filename,
                    )
                )
                count += 1

        print(f"Read {count:4d} artifacts from {subdir}")

    return packages


def group_by_name(packages):
    grouped = defaultdict(list)
    for package in packages:
        grouped[package.name].append(package)
    return grouped


def package_order(package: Package):
    """Order an artifact by Conda version first, then by build number."""
    return VersionOrder(package.version), package.build_number


def latest_release(packages):
    """Return the latest (version, build_number) pair in a package group."""
    latest = max(packages, key=package_order)
    return latest.version, latest.build_number


def delete_package(package: Package, owner: str) -> bool:
    spec = package.spec(owner)
    cmd = ["anaconda", "remove", spec, "--force"]
    result = sp.run(cmd, capture_output=True, text=True, check=False)

    if result.returncode != 0:
        print(f"  ERROR deleting {spec}", file=sys.stderr)
        if result.stderr.strip():
            print(f"  {result.stderr.strip()}", file=sys.stderr)
        return False

    print(f"  Deleted: {spec}")
    return True


def run(owner: str, label: str, subdirs, execute: bool) -> int:
    print(f"Reading {owner}/label/{label} ...")
    packages = get_packages(owner, label, subdirs)
    grouped = group_by_name(packages)

    candidates = 0
    deleted = 0
    failed = 0

    for name in sorted(grouped):
        artifacts = grouped[name]
        keep_version, keep_build_number = latest_release(artifacts)
        keep = [
            p
            for p in artifacts
            if p.version == keep_version and p.build_number == keep_build_number
        ]
        remove = [
            p
            for p in artifacts
            if p.version != keep_version or p.build_number != keep_build_number
        ]

        if not remove:
            continue

        print(f"\n{name}:")
        print(
            f"  Keeping version {keep_version}, build number "
            f"{keep_build_number} ({len(keep)} artifact(s))"
        )
        for package in sorted(
            keep, key=lambda p: (p.subdir, p.build_number, p.build, p.filename)
        ):
            print(f"    {package.subdir}/{package.filename}")

        for package in sorted(
            remove,
            key=lambda p: (
                VersionOrder(p.version),
                p.build_number,
                p.subdir,
                p.build,
                p.filename,
            ),
        ):
            candidates += 1
            spec = package.spec(owner)
            if execute:
                if delete_package(package, owner):
                    deleted += 1
                else:
                    failed += 1
            else:
                print(f"  Would delete: {spec}")

    print("\nSummary:")
    print(f"  Found {len(packages)} artifact(s) for {len(grouped)} package(s)")
    if execute:
        print(f"  Deleted {deleted}/{candidates} old artifact(s)")
        if failed:
            print(f"  Failed to delete {failed} artifact(s)")
    else:
        print(f"  Would delete {candidates} old artifact(s)")
        print("  Dry run only; pass --execute to perform deletions")

    return 1 if failed else 0


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Delete old versions and builds from an Anaconda label, keeping "
            "every artifact whose version and build number match the latest "
            "release of each package."
        )
    )
    parser.add_argument(
        "label",
        help="Label/channel to clean, for example: dev",
    )
    parser.add_argument(
        "--owner",
        default=DEFAULT_OWNER,
        help=f"Anaconda owner (default: {DEFAULT_OWNER})",
    )
    parser.add_argument(
        "--subdir",
        dest="subdirs",
        action="append",
        help=(
            "Subdir to read; repeat for multiple subdirs. If omitted, the "
            "standard noarch, Linux, macOS, and Windows subdirs are read."
        ),
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Perform deletions. The default is a dry run.",
    )
    args = parser.parse_args()

    subdirs = tuple(dict.fromkeys(args.subdirs or DEFAULT_SUBDIRS))
    try:
        return run(args.owner, args.label, subdirs, args.execute)
    except (RuntimeError, OSError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
