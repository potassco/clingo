import json
import os
import re
import subprocess
import sys
import tomllib
import urllib.error
import urllib.parse
import urllib.request
from argparse import ArgumentParser
from configparser import ConfigParser
from pathlib import Path


class Config:
    def __init__(self) -> None:
        """
        Initialize the Config object by reading the config.ini file.

        The config.ini file is expected to be located in the same directory as
        this script.
        """
        config_path = Path(__file__).resolve().with_name("config.ini")
        self.root = config_path.parent.parent
        self.cfg = ConfigParser()

        try:
            with config_path.open(encoding="utf-8") as fh:
                self.cfg.read_file(fh)
        except OSError as exc:
            raise RuntimeError(f"cannot read config file {config_path}: {exc}") from exc

    def get_value(self, section: str, entry: str, default: str | None) -> str:
        """
        Get a value from the config files.

        This method retrieves the value of a specified entry from a given
        section in the config file. A default value can be provided, which will
        be returned if the section or entry is not found. If the entry is not
        found and no default is provided, a RuntimeError is raised.
        """
        if section not in self.cfg:
            if default is not None:
                return default
            raise RuntimeError(f"missing section [{section}]")
        config_section = self.cfg[section]

        if entry not in config_section:
            if default is not None:
                return default
            raise RuntimeError(f"missing entry {entry}")
        return config_section[entry]

    def get_pypi_name(self) -> str:
        """
        Get the package name from the pyproject.toml file.
        """
        pyproject = self.root / "pyproject.toml"
        try:
            data = tomllib.loads(pyproject.read_text(encoding="utf-8"))
        except OSError as exc:
            raise RuntimeError(f"cannot read {pyproject}: {exc}") from exc
        except tomllib.TOMLDecodeError as exc:
            raise RuntimeError(f"invalid TOML in {pyproject}: {exc}") from exc

        name = data.get("project", {}).get("name")
        if isinstance(name, str) and name.strip():
            return name.strip()

        raise RuntimeError("package name not found in [project].name")

    def pypi_build_number(self, index: str) -> int:
        """
        Get the next build number for a PyPI package.
        """
        package_name = self.get_pypi_name()
        base_version = self.get_version()

        index_base = (
            "https://test.pypi.org/pypi"
            if index == "test.pypi"
            else "https://pypi.org/pypi"
        )
        url = f"{index_base}/{urllib.parse.quote(package_name)}/json"

        try:
            with urllib.request.urlopen(url, timeout=10) as response:
                data = json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as err:
            if err.code == 404:
                print("pypi!")
                return 0
            raise RuntimeError(
                f"failed to fetch release metadata from {url}: {err}"
            ) from err
        except (
            urllib.error.URLError,
            TimeoutError,
            json.JSONDecodeError,
            OSError,
        ) as err:
            raise RuntimeError(
                f"failed to fetch release metadata from {url}: {err}"
            ) from err

        releases = data.get("releases", {})
        if not isinstance(releases, dict):
            return 0

        pattern = re.compile(rf"^{re.escape(base_version)}(?:\.post([0-9]+))?$")
        found = []
        for release_version in releases:
            match = pattern.match(release_version)
            if match:
                found.append(int(match.group(1) or 0))

        return max(found) + 1 if found else 0

    def conda_build_number(self, label: str) -> int:
        """
        Compute the next build number for a conda package.

        The build number is determined by inspecting package files on
        anaconda.org independent of platform. If no matching build numbers are
        found, the result is 0.

        For the `main` label, the next number is one greater than the maximum
        build number found for the specified version among main packages only.
        For non-main labels, the next number is one greater than the maximum
        build number found for the specified version across all labels.

        This avoids collisions for development labels, while still allowing
        `main` to override non-main packages. Development packages for a
        version are expected to be deleted once the main package is released.
        """

        owner = self.get_value("conda", "owner", None)
        package = self.get_value("conda", "name", None)
        version = self.get_version()

        url = f"https://api.anaconda.org/package/{owner}/{package}/files"

        try:
            with urllib.request.urlopen(url) as response:
                files = json.load(response)
        except (urllib.error.URLError, json.JSONDecodeError) as exc:
            raise RuntimeError(f"could not retrieve package files: {exc}") from exc

        build_numbers = [
            entry["attrs"]["build_number"]
            for entry in files
            if entry.get("version") == version
            and (label != "main" or label in entry.get("labels") or [])
            and isinstance(entry.get("attrs", {}).get("build_number", 0), int)
        ]

        return max(build_numbers) + 1 if build_numbers else 0

    def ppa_build_number(self, repository: str) -> int:
        """
        Get the next build number for a PPA package.
        """
        owner = self.get_value("ppa", "owner", None)
        name = self.get_value("ppa", "name", None)

        url = (
            f"http://ppa.launchpad.net/{owner}/{repository}/ubuntu/pool/main/c/{name}/"
        )
        with urllib.request.urlopen(url, timeout=20) as r:
            html = r.read().decode("utf-8", errors="replace")

        version = self.get_version()
        pattern = re.compile(rf"{re.escape(version)}-[a-z]+(\d+)\.dsc")
        nums = [int(m.group(1)) for m in pattern.finditer(html)]
        current = max(nums) if nums else None

        return 0 if current is None else current + 1

    def cloudsmith_build_number(self, repository: str) -> int:
        """
        Get the next build number for a Cloudsmith package.

        The cloudsmith CLI must be installed and configured with an API key for
        this
        """
        owner = self.get_value("cloudsmith", "owner", None)
        name = self.get_value("cloudsmith", "name", None)
        version = self.get_version()

        cmd = [
            "cloudsmith",
            "list",
            "packages",
            f"{owner}/{repository}",
            "--output-format",
            "json",
            "--query",
            f"name:{name}",
        ]

        try:
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            payload = json.loads(result.stdout)
        except (subprocess.CalledProcessError, json.JSONDecodeError) as exc:
            raise RuntimeError(
                f"could not retrieve cloudsmith packages: {exc}"
            ) from exc

        packages = (
            payload.get("data", payload) if isinstance(payload, dict) else payload
        )
        if not isinstance(packages, list):
            raise TypeError("unexpected cloudsmith JSON format")

        pattern = re.compile(rf"^{re.escape(version)}(?:-[a-z]+|[.])(\d+)$")

        nums = []
        for p in packages:
            if not isinstance(p, dict) or p.get("name") != name:
                continue
            v = p.get("version")
            if not isinstance(v, str):
                continue
            m = pattern.match(v)
            if m:
                nums.append(int(m.group(1)))

        return max(nums) + 1 if nums else 0

    def get_version(self) -> str:
        """
        Compute the version of the package from the configured header file.
        """
        field = self.get_value("project", "macro_field", None)
        header_raw = self.get_value("project", "header_path", None)
        header_path = Path(header_raw)
        if not header_path.is_absolute():
            header_path = (self.root / header_path).resolve()

        pattern = re.compile(
            rf'#define\s+{re.escape(field)}\s+"([0-9]+\.[0-9]+\.[0-9]+)"'
        )

        try:
            with header_path.open(encoding="utf-8") as fh:
                for line in fh:
                    match = pattern.search(line)
                    if match:
                        return match.group(1)
        except OSError as exc:
            raise RuntimeError(f"cannot read header file {header_path}: {exc}") from exc

        raise RuntimeError(f"{field} not found in {header_path}")

    def write_git_hash(self):
        """
        Write a git hash file using GITHUB_SHA or git rev-parse HEAD.
        """
        git_hash = os.environ.get("GITHUB_SHA", "").strip()
        if git_hash:
            if not re.fullmatch(r"[0-9a-fA-F]{7,}", git_hash):
                raise RuntimeError(f"invalid GITHUB_SHA: {git_hash!r}")
            git_hash = git_hash[:7]
        else:
            try:
                result = subprocess.run(
                    ["git", "-C", str(self.root), "rev-parse", "--short", "HEAD"],
                    check=True,
                    capture_output=True,
                    text=True,
                )
                git_hash = result.stdout.strip()
            except (subprocess.CalledProcessError, OSError) as exc:
                raise RuntimeError(
                    "cannot determine git hash from GITHUB_SHA or git"
                ) from exc

        if not re.fullmatch(r"[0-9a-fA-F]{7,40}", git_hash):
            raise RuntimeError(f"invalid git hash: {git_hash!r}")

        output_path = (self.root / Path(".git_hash")).resolve()
        try:
            output_path.write_text(f"{git_hash}\n", encoding="utf-8")
        except OSError as exc:
            raise RuntimeError(
                f"cannot write git hash file {output_path}: {exc}"
            ) from exc

        print(f"Wrote git hash {git_hash} to '{output_path.relative_to(self.root)}'.")

    def main(self) -> int:
        """
        Run the config script.
        """
        parser = ArgumentParser()
        subparsers = parser.add_subparsers(dest="command", required=True)

        build_parser = subparsers.add_parser(
            "build-number", help="Compute next build number"
        )
        build_subparsers = build_parser.add_subparsers(
            dest="build_target", required=True
        )

        pypi_parser = build_subparsers.add_parser(
            "pypi", help="Compute next PyPI build number"
        )
        pypi_parser.add_argument(
            "index",
            choices=("pypi", "test.pypi"),
            help="Package index to inspect",
        )

        conda_parser = build_subparsers.add_parser(
            "conda", help="Compute next conda build number"
        )
        conda_parser.add_argument("label", nargs="?", default="main")

        ppa_parser = build_subparsers.add_parser(
            "ppa", help="Compute next PPA build number for <repository>"
        )
        ppa_parser.add_argument("repository")

        cloudsmith_parser = build_subparsers.add_parser(
            "cloudsmith", help="Compute next Cloudsmith build number for <repository>"
        )
        cloudsmith_parser.add_argument("repository")

        config_parser = subparsers.add_parser(
            "get-config", help="Read value from config: <section> <entry> [default]"
        )
        config_parser.add_argument("section")
        config_parser.add_argument("entry")
        config_parser.add_argument("default", nargs="?")

        subparsers.add_parser("get-version", help="Read version from configured header")

        subparsers.add_parser(
            "write-git-hash",
            help="Write git hash file from GITHUB_SHA or git rev-parse",
        )

        try:
            args = parser.parse_args()

            if args.command == "build-number":
                if args.build_target == "pypi":
                    print(self.pypi_build_number(args.index))
                elif args.build_target == "conda":
                    print(self.conda_build_number(args.label))
                elif args.build_target == "ppa":
                    print(self.ppa_build_number(args.repository))
                elif args.build_target == "cloudsmith":
                    print(self.cloudsmith_build_number(args.repository))
            elif args.command == "get-config":
                print(self.get_value(args.section, args.entry, args.default))
            elif args.command == "get-version":
                print(self.get_version())
            elif args.command == "write-git-hash":
                self.write_git_hash()

            return 0
        except RuntimeError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1


if __name__ == "__main__":
    raise SystemExit(Config().main())
