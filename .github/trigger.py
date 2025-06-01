#!/usr/bin/env python3
"""Simple script to dispatch workflows."""

import urllib.request
import urllib.error
import json
import os
import sys
import argparse

REPO = "clingo"
OWNER = "potassco"
API_URL = f"https://api.github.com/repos/{OWNER}/{REPO}"
TOKEN_FILE = os.path.expanduser("~/.tokens")
WORKFLOW_ID_RELEASE = "165237057"  # Adjust as needed


def get_token():
    """Extract the workflow_dispatch token from ~/.tokens."""
    try:
        with open(TOKEN_FILE, encoding="utf-8") as f:
            lines = f.readlines()
        idx = lines.index("workflow_dispatch\n")
        return lines[idx + 1].strip()
    except Exception as e:  # pylint: disable=broad-exception-caught
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
    """List all github workflows"""
    url = f"{API_URL}/actions/workflows"
    resp = make_request(url)
    data = json.loads(resp)
    for wf in data.get("workflows", []):
        print(f"{wf['id']}: {wf['name']}")


def dispatch_workflow(workflow_id: str, ref: str, label: str, build: str):
    """Dispatch a workflow event"""
    url = f"{API_URL}/actions/workflows/{workflow_id}/dispatches"
    payload = {"ref": ref, "inputs": {"label": label, "build_number": build}}
    make_request(url, method="POST", data=payload)
    print(f"Workflow dispatched: https://github.com/{OWNER}/{REPO}/actions")


def main():
    """
    Run the script.
    """
    parser = argparse.ArgumentParser(
        description="Trigger GitHub Actions workflows for clingo (urllib.request version)."
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
    elif args.command == "release":
        dispatch_workflow(WORKFLOW_ID_RELEASE, args.branch, "main", args.build_number)
    elif args.command == "dev":
        dispatch_workflow(WORKFLOW_ID_RELEASE, args.branch, "dev-20", "auto")
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
