#!/usr/bin/env python3
"""Simple script to dispatch workflows."""

import argparse
import json
import os
import sys
import urllib.error
import urllib.request

from config import Config

TOKEN_FILE = os.path.expanduser("~/.tokens")

WORKFLOWS = [
    "conda",
    "pypi",
    "ppa",
    "cloudsmith",
]

WORKFLOW_INPUTS = {
    "release": {
        "conda": {"label": "main"},
        "ppa": {"type": "stable"},
        "cloudsmith": {"type": "stable"},
    },
    "dev": {
        "conda": {"label": "dev-20"},
        "ppa": {"type": "wip-20"},
        "cloudsmith": {"type": "wip-20"},
    },
}


class Trigger:
    def __init__(self):
        self.config = Config()
        self.repo = self.config.get_value("project", "name", None)
        self.owner = self.config.get_value("project", "owner", None)
        self.api_url = f"https://api.github.com/repos/{self.owner}/{self.repo}"
        self.workflow_ids = {}
        for workflow in WORKFLOWS:
            self.workflow_ids[workflow] = self.config.get_value(
                workflow, "workflow_id", None
            )

    def get_token(self):
        """Extract the workflow_dispatch token from ~/.tokens."""
        try:
            with open(TOKEN_FILE, encoding="utf-8") as f:
                lines = f.readlines()
            idx = lines.index("workflow_dispatch\n")
            return lines[idx + 1].strip()
        except (FileNotFoundError, IndexError, ValueError, OSError) as e:
            print(f"Failed to read token: {e}", file=sys.stderr)
            sys.exit(1)

    def make_request(self, url, method="GET", data=None):
        """Make a github API request."""
        token = self.get_token()
        headers = {
            "Accept": "application/vnd.github.v3+json",
            "Authorization": f"token {token}",
            "User-Agent": "python-urllib",
        }
        if data is not None:
            data = json.dumps(data).encode("utf-8")
            headers["Content-Type"] = "application/json"
        req = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(req) as response:
                return response.read().decode()
        except urllib.error.HTTPError as e:
            print(
                f"HTTP Error: {e.code} {e.reason}\n{e.read().decode()}", file=sys.stderr
            )
            sys.exit(1)
        except urllib.error.URLError as e:
            print(f"URL Error: {e.reason}", file=sys.stderr)
            sys.exit(1)

    def list_workflows(self):
        """List all github workflows."""
        url = f"{self.api_url}/actions/workflows"
        resp = self.make_request(url)
        data = json.loads(resp)
        for wf in data.get("workflows", []):
            print(f"{wf['id']}: {wf['name']}")

    def dispatch_workflow(self, workflow_id: str, ref: str, inputs: dict):
        """Dispatch a workflow event."""
        url = f"{self.api_url}/actions/workflows/{workflow_id}/dispatches"
        payload = {"ref": ref, "inputs": inputs}
        self.make_request(url, method="POST", data=payload)
        print(
            f"Workflow dispatched: https://github.com/{self.owner}/{self.repo}/actions"
        )

    def trigger_workflows(self, command: str, branch: str, workflows: list[str]):
        """Trigger selected workflows for a command type."""
        for workflow in workflows:
            inputs = {
                "wip": command == "dev",
                **WORKFLOW_INPUTS[command].get(workflow, {}),
            }
            self.dispatch_workflow(self.workflow_ids[workflow], branch, inputs)

    def parse_workflows(self, value: str) -> list[str]:
        """Parse comma-separated workflow names."""
        selected = list(
            dict.fromkeys(item.strip() for item in value.split(",") if item.strip())
        )
        if not selected:
            raise argparse.ArgumentTypeError(
                f"At least one workflow is required. Valid: {', '.join(sorted(WORKFLOWS))}"
            )
        invalid = [w for w in selected if w not in WORKFLOWS]
        if invalid:
            raise argparse.ArgumentTypeError(
                f"Invalid workflow(s): {', '.join(invalid)}. "
                f"Valid: {', '.join(sorted(WORKFLOWS))}"
            )
        return selected

    def main(self):
        """Run the script."""
        parser = argparse.ArgumentParser(
            description="Trigger GitHub Actions workflows for clingo."
        )
        parser.add_argument(
            "--workflows",
            type=self.parse_workflows,
            default=WORKFLOWS,
            help="Comma-separated workflows (default: conda,pypi,ppa,cloudsmith)",
        )
        subparsers = parser.add_subparsers(dest="command", required=True)

        subparsers.add_parser("list", help="List available workflows")

        parser_release = subparsers.add_parser(
            "release", help="Deploy release packages"
        )
        parser_release.add_argument("branch", help="Branch to deploy from")

        parser_dev = subparsers.add_parser("dev", help="Deploy development packages")
        parser_dev.add_argument("branch", help="Branch to deploy from")

        args = parser.parse_args()

        if args.command == "list":
            self.list_workflows()
        elif args.command in {"release", "dev"}:
            self.trigger_workflows(args.command, args.branch, args.workflows)
        else:
            parser.print_help()
            sys.exit(1)


if __name__ == "__main__":
    Trigger().main()
