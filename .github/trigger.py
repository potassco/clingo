#!/usr/bin/env python3
"""Simple script to dispatch workflows."""

import argparse
import json
import os
import sys
import urllib.error
import urllib.request

REPO = "clingo"
OWNER = "potassco"
API_URL = f"https://api.github.com/repos/{OWNER}/{REPO}"
TOKEN_FILE = os.path.expanduser("~/.tokens")

WORKFLOW_IDS = {
    "conda": "165237057",
    "pypi": "189686367",
    "ppa": "190109021",
    "cloudsmith": "191817760",
}

WORKFLOW_INPUTS = {
    "release": {
        "conda": {"label": "main"},
        "pypi": {"index": "pypi"},
        "ppa": {"type": "stable"},
        "cloudsmith": {"type": "stable"},
    },
    "dev": {
        "conda": {"label": "dev-20"},
        "pypi": {"index": "testpypi"},
        "ppa": {"type": "wip-20"},
        "cloudsmith": {"type": "wip-20"},
    },
}


def get_token():
    """Extract the workflow_dispatch token from ~/.tokens."""
    try:
        with open(TOKEN_FILE, encoding="utf-8") as f:
            lines = f.readlines()
        idx = lines.index("workflow_dispatch\n")
        return lines[idx + 1].strip()
    except (FileNotFoundError, IndexError, ValueError, OSError) as e:
        print(f"Failed to read token: {e}", file=sys.stderr)
        sys.exit(1)


def make_request(url, method="GET", data=None):
    """Make a github API request."""
    token = get_token()
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
        print(f"HTTP Error: {e.code} {e.reason}\n{e.read().decode()}", file=sys.stderr)
        sys.exit(1)
    except urllib.error.URLError as e:
        print(f"URL Error: {e.reason}", file=sys.stderr)
        sys.exit(1)


def list_workflows():
    """List all github workflows."""
    url = f"{API_URL}/actions/workflows"
    resp = make_request(url)
    data = json.loads(resp)
    for wf in data.get("workflows", []):
        print(f"{wf['id']}: {wf['name']}")


def dispatch_workflow(workflow_id: str, ref: str, inputs: dict):
    """Dispatch a workflow event."""
    url = f"{API_URL}/actions/workflows/{workflow_id}/dispatches"
    payload = {"ref": ref, "inputs": inputs}
    make_request(url, method="POST", data=payload)
    print(f"Workflow dispatched: https://github.com/{OWNER}/{REPO}/actions")


def trigger_workflows(
    command: str, branch: str, build_number: str, workflows: list[str]
):
    """Trigger selected workflows for a command type."""
    for workflow in workflows:
        inputs = {"build_number": build_number, **WORKFLOW_INPUTS[command][workflow]}
        dispatch_workflow(WORKFLOW_IDS[workflow], branch, inputs)


def parse_workflows(value: str) -> list[str]:
    """Parse comma-separated workflow names."""
    selected = list(
        dict.fromkeys(item.strip() for item in value.split(",") if item.strip())
    )
    if not selected:
        raise argparse.ArgumentTypeError(
            f"At least one workflow is required. Valid: {', '.join(sorted(WORKFLOW_IDS))}"
        )
    invalid = [w for w in selected if w not in WORKFLOW_IDS]
    if invalid:
        raise argparse.ArgumentTypeError(
            f"Invalid workflow(s): {', '.join(invalid)}. "
            f"Valid: {', '.join(sorted(WORKFLOW_IDS))}"
        )
    return selected


def main():
    """Run the script."""
    parser = argparse.ArgumentParser(
        description="Trigger GitHub Actions workflows for clingo."
    )
    parser.add_argument(
        "--workflows",
        type=parse_workflows,
        default=list(WORKFLOW_IDS),
        help="Comma-separated workflows (default: conda,pypi,ppa,cloudsmith)",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("list", help="List available workflows")

    parser_release = subparsers.add_parser("release", help="Deploy release packages")
    parser_release.add_argument("branch", help="Branch to deploy from")
    parser_release.add_argument("build_number", help="Build number to use")

    parser_dev = subparsers.add_parser("dev", help="Deploy development packages")
    parser_dev.add_argument("branch", help="Branch to deploy from")

    args = parser.parse_args()

    if args.command == "list":
        list_workflows()
    elif args.command in {"release", "dev"}:
        build_number = args.build_number if args.command == "release" else "auto"
        trigger_workflows(args.command, args.branch, build_number, args.workflows)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
