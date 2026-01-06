import os
from pathlib import Path
import subprocess
from typing import List
import argparse
import urllib.request
import urllib.error
import json

filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

DEFAULT_MODEL = "freehuntx/qwen3-coder:14b"
OLLAMA_API_URL = "http://localhost:11434/api/generate"
DEFAULT_PROMPT = "Analyze this code for bugs, security issues, and improvements. Include file names and line numbers for referenced content. List anything that could be a false positive as such. For C/C++ files, ignore header guards, relative path includes, forward declarations, and clear use of type-erased void* pointers."


def parse_args():
    parser = argparse.ArgumentParser(description="LLM-based code analysis using Ollama")
    parser.add_argument("--model", type=str, default=DEFAULT_MODEL, help="Ollama model to use")
    parser.add_argument("--files", nargs="+", help="Files to analyze")
    parser.add_argument("--diff", action="store_true", help="Analyze git diff (staged changes)")
    parser.add_argument("--pr", type=str, help="Analyze PR diff against base branch (e.g., 'main')")
    parser.add_argument("--prompt", type=str, default=DEFAULT_PROMPT,
                        help="Custom analysis prompt")
    return parser.parse_args()


def read_files(file_paths: List[str]) -> str:
    content_parts = []
    for path in file_paths:
        try:
            with open(path, 'r') as f:
                content = f.read()
            content_parts.append(f"=== File: {path} ===\n{content}\n")
        except Exception as e:
            print(e)
            exit(-1)
    return "\n".join(content_parts)


def get_git_diff(staged_only: bool = True) -> str:
    cmd = ["git", "diff", "--cached"] if staged_only else ["git", "diff"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout


def get_pr_diff(base_branch: str = "main") -> str:
    # where branches diverged
    merge_base = subprocess.run(
        ["git", "merge-base", base_branch, "HEAD"],
        capture_output=True, text=True
    ).stdout.strip()

    result = subprocess.run(
        ["git", "diff", merge_base, "HEAD"],
        capture_output=True, text=True
    )
    return result.stdout


def query_ollama_streaming(model: str, prompt: str, context: str):
    # this all works I guess
    full_prompt = f"{prompt}\n\n{context}"

    payload = {
        "model": model,
        "prompt": full_prompt,
        "stream": True
    }

    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(
        OLLAMA_API_URL,
        data=data,
        headers={'Content-Type': 'application/json'}
    )

    try:
        with urllib.request.urlopen(req, timeout=300) as response:
            for line in response:
                if line:
                    chunk = json.loads(line.decode('utf-8'))
                    token = chunk.get("response", "")
                    print(token, end="", flush=True)
                    if chunk.get("done", False):
                        break
            print()
    except urllib.error.URLError as e:
        if isinstance(e.reason, ConnectionRefusedError):
            print("Error: Cannot connect to Ollama. Is it running? (ollama serve)")
        else:
            print(f"Error: {e.reason}")
    except TimeoutError:
        print("Error: Request timed out")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    args = parse_args()

    context = ""

    if args.files:
        context = read_files(args.files)
    elif args.pr:
        context = get_pr_diff(args.pr)
        if not context.strip():
            print(f"No diff found between {args.pr} and HEAD")
            exit(1)
    elif args.diff:
        context = get_git_diff(staged_only=True)
        if not context.strip():
            # Fall back to unstaged diff
            context = get_git_diff(staged_only=False)
        if not context.strip():
            print("No changes to analyze")
            exit(1)
    else:
        print("Specify --files, --diff, or --pr to provide code for analysis")
        exit(1)

    print(f"Using model: {args.model}")
    print(f"Analyzing {len(context)} characters of content...\n")

    # Stream the response for better UX
    query_ollama_streaming(args.model, args.prompt, context)
