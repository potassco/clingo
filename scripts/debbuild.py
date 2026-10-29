#!/usr/bin/env python3

import argparse
import glob
import re
import subprocess
import sys
from datetime import datetime
from email.utils import format_datetime
from pathlib import Path

from config import Config

VALID_TARGETS = {"ppa", "cloudsmith"}
VALID_REPOSITORIES = {"stable", "wip-20"}
UBUNTU_CODENAMES = {"noble", "resolute"}
DEBIAN_CODENAMES = {"trixie"}
VALID_ACTIONS = {"prepare", "create", "sync", "changes", "build", "put"}


class DebBuilder:
    def __init__(self, target, repository, codename) -> None:
        self.repository = repository
        self.codename = codename
        self.config = Config()
        self.target = target
        self.root = self.config.root
        self.name = self.config.get_value(self.target, "name", None)
        self.version = self.config.get_version()
        self.owner = self.config.get_value(self.target, "owner", None)

    def run(
        self, cmd: list[str], cwd: Path | None = None, capture: bool = False
    ) -> str:
        result = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else None,
            check=True,
            text=True,
            stdout=subprocess.PIPE if capture else None,
        )
        return result.stdout if capture else ""

    def get_config_lines(self, key: str, default: str | None) -> list[str]:
        out = self.config.get_value("deb", key, default)
        return [line for line in out.splitlines() if line.strip()]

    def extract_version(self) -> str:
        changelog = (
            Path(self.root)
            / "build"
            / self.codename
            / "source"
            / "debian"
            / "changelog"
        )
        first = changelog.read_text(encoding="utf-8").splitlines()[0]
        m = re.match(r"^\S+\s+\(([^)]+)\)\s+\S+;\s+urgency=", first)
        if not m:
            raise RuntimeError(f"could not parse version from {changelog}: {first!r}")
        return m.group(1)

    def action_prepare(self) -> None:
        deps = self.get_config_lines("extra_depends", "")
        self.run(["sudo", "apt-get", "update"])
        args = [
            "sudo",
            "apt-get",
            "install",
            "pbuilder",
            "pbuilder-scripts",
            "debootstrap",
            "devscripts",
            "dput",
            "dh-make",
            *deps,
        ]
        if self.codename in DEBIAN_CODENAMES:
            args.append("debian-archive-keyring")
        self.run(args)

    def action_create(self) -> None:
        base = f"/var/cache/pbuilder/{self.repository}-{self.codename}.tgz"
        mirror = (
            "http://deb.debian.org/debian"
            if self.codename in DEBIAN_CODENAMES
            else "http://archive.ubuntu.com/ubuntu"
        )
        self.run(
            [
                "sudo",
                "pbuilder",
                "create",
                "--basetgz",
                base,
                "--mirror",
                mirror,
                "--distribution",
                self.codename,
                "--debootstrapopts",
                "--variant=buildd",
            ]
        )
        repository = ""
        if self.config.get_value("deb", "add_repository", "no") == "yes":
            repository = f"{self.owner}/{self.repository}"

        self.run(
            [
                "sudo",
                "pbuilder",
                "execute",
                "--basetgz",
                base,
                "--save-after-exec",
                "--",
                "scripts/debbuild_prepare.sh",
                self.target,
                repository,
            ]
        )

    def action_sync(self) -> None:
        include = self.get_config_lines("include", None)
        exclude = self.get_config_lines("exclude", None)

        target = Path(self.root) / "build" / self.codename / "source"
        target.mkdir(parents=True, exist_ok=True)
        rsync_args = [
            "rsync",
            "-aq",
            "--exclude",
            ".git*",
            f"{Path(self.root) / 'scripts' / 'deb' / self.codename}/",
            f"{target}/",
        ]
        self.run(rsync_args)

        sources: list[str] = []
        for pattern in include:
            sources.extend(glob.glob(pattern))

        rsync_args = ["rsync", "-aq"]
        for x in exclude:
            rsync_args.extend(["--exclude", x])
        rsync_args.extend(sources)
        rsync_args.append(f"{target}/")

        if sources:
            self.run(rsync_args)
        self.run(["cp", ".git_hash", f"{target}/.git_hash"])

    def action_changes(self) -> None:
        if self.target == "ppa":
            build = self.config.ppa_build_number(self.repository)
        else:
            build = self.config.cloudsmith_build_number(self.repository)

        suffix = f"+{self.codename}{build}"

        changelog = (
            Path(self.root)
            / "build"
            / self.codename
            / "source"
            / "debian"
            / "changelog"
        )
        revision = Path(self.root / ".git_hash").read_text(encoding="utf-8").strip()

        changelog.write_text(
            (
                f"{self.name} ({self.version}{suffix}) {self.codename}; urgency=medium\n\n"
                f"  * build for git revision {revision}\n\n"
                f" -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  "
                f"{format_datetime(datetime.now().astimezone())}\n"
            ),
            encoding="utf-8",
        )

    def action_build(self) -> None:
        cwd = Path(self.root) / "build" / self.codename / "source"
        (cwd.parent / "binary").mkdir(exist_ok=True)
        self.run(
            [
                "pdebuild",
                "--buildresult",
                "../binary",
                "--",
                "--basetgz",
                f"/var/cache/pbuilder/{self.repository}-{self.codename}.tgz",
            ],
            cwd,
        )
        if self.target == "ppa":
            key = self.config.get_value("ppa", "key", None)
            version = self.extract_version()
            self.run(
                [
                    "debsign",
                    "--re-sign",
                    f"-k{key}",
                    f"{self.name}_{version}_source.changes",
                ],
                cwd.parent,
            )

    def action_put(self) -> None:
        if self.target == "ppa":
            cwd = Path(self.root) / "build" / self.codename
            debs = sorted(cwd.glob(f"{self.name}_*_source.changes"))
            if len(debs) != 1:
                raise RuntimeError(
                    f"expected exactly one .changes file under {cwd}, found: {debs}"
                )
            self.run(
                [
                    "dput",
                    f"ppa:{self.owner}/{self.repository}",
                    debs[0].name,
                ],
                cwd,
            )
        else:
            binary_root = Path(self.root) / "build" / self.codename / "binary"
            debs = sorted(binary_root.glob("*.deb"))
            if not debs:
                raise RuntimeError(f"no .deb files found under {binary_root}")

            os_name = "ubuntu" if self.codename in UBUNTU_CODENAMES else "debian"
            cloudsmith_repo = (
                f"{self.owner}/{self.repository}/{os_name}/{self.codename}"
            )
            for debfile in debs:
                self.run(["cloudsmith", "push", "deb", cloudsmith_repo, str(debfile)])


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog=Path(sys.argv[0]).name,
        description="Build and publish Debian source packages for Potassco PPAs.",
    )
    parser.add_argument("target", choices=sorted(VALID_TARGETS))
    parser.add_argument("repository", choices=sorted(VALID_REPOSITORIES))
    parser.add_argument(
        "codename", choices=sorted(list(UBUNTU_CODENAMES) + list(DEBIAN_CODENAMES))
    )
    parser.add_argument("actions", nargs="*", metavar="action")
    return parser


def main() -> int:
    parser = build_parser()
    ns = parser.parse_args()

    if ns.target == "ppa" and ns.codename not in UBUNTU_CODENAMES:
        parser.error(
            f"codename '{ns.codename}' is not valid for target 'ppa' (expected one of: {', '.join(sorted(UBUNTU_CODENAMES))})"
        )

    builder = DebBuilder(ns.target, ns.repository, ns.codename)

    for act in ns.actions:
        print(act)
        if act not in VALID_ACTIONS:
            parser.error(
                f"invalid action '{act}' (expected one of: {', '.join(sorted(VALID_ACTIONS))})"
            )

        if act == "prepare":
            builder.action_prepare()
        elif act == "create":
            builder.action_create()
        elif act == "sync":
            builder.action_sync()
        elif act == "changes":
            builder.action_changes()
        elif act == "build":
            builder.action_build()
        elif act == "put":
            builder.action_put()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
