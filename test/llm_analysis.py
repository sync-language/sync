import os
from pathlib import Path
import subprocess
from typing import List, Set
import argparse
import urllib.request
import urllib.error
import json
import re

filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

DEFAULT_MODEL = "deepseek-r1:14b"
OLLAMA_API_URL = "http://localhost:11434/api/generate"
INCLUDE_DEPTH_LIMIT = 3
EXCLUDED_HEADERS = {"doctest.h"}
COMPRESS_REFERENCE_HEADERS = True

DEFAULT_PROMPT = """You are a security auditor and bug hunter analyzing C/C++ code for a cross-platform embeddable language compiler and runtime called Sync.

## Primary Goal
Find bugs, vulnerabilities, and defects. Report potential issues even if uncertain - humans will audit false positives. Do NOT explain how the code works unless it clarifies an issue.

## Issue Categories (in priority order)
1. **Memory Safety**: buffer overflows, use-after-free, double-free, null dereference, uninitialized reads, memory leaks
2. **Undefined Behavior**: signed overflow, invalid pointer arithmetic, strict aliasing violations, sequence point violations
3. **Thread Safety**: data races, deadlocks, lock ordering issues, missing synchronization, TOCTOU
4. **Security**: integer overflow/underflow leading to exploits, format string bugs, injection vectors
5. **Logic Errors**: off-by-one, incorrect bounds, wrong operator, unreachable code, dead stores

## Output Format
Each source line is prefixed with "filename:line_number:" - use these EXACT paths in your output.
For EACH issue (report all, even uncertain ones):
```
[CRITICAL|WARNING|UNCERTAIN] <copy exact filename:line from source>
Issue: <one-line description>
Why: <brief explanation of the bug or risk>
Fix: <suggested fix if obvious, or "Needs review">
```

## Scope Rules
- ONLY analyze PRIMARY FILES and DIFF sections
- REFERENCE HEADERS are context only - do NOT report issues in them
- Focus analysis on DIFF changes, use PRIMARY FILES for full context

## Ignore
- Header guards, forward declarations, intentional void* type erasure
- Style issues, missing docs, not using std containers
- Issues in REFERENCE HEADERS section
- Not using C++ std
- Lack of exceptions, as -fno-exceptions must be supported

## Be Thorough
- Report edge cases that MIGHT fail
- Flag risky patterns even if currently safe
- Mark uncertain findings as [UNCERTAIN] for human review

## IMPORTANT
Do NOT summarize or explain the code. ONLY output a list of issues in the format above. Start your response with the first issue immediately."""

OUTPUT_PRIMER = """
=== BEGIN SECURITY ANALYSIS ===

Analyzing for: memory safety, undefined behavior, thread safety, security, logic errors.
"""


def parse_args():
    parser = argparse.ArgumentParser(description="LLM-based code analysis using Ollama")
    parser.add_argument("--model", type=str, default=DEFAULT_MODEL, help="Ollama model to use")
    parser.add_argument("--files", nargs="+", help="Files to analyze")
    parser.add_argument("--diff", action="store_true", help="Analyze git diff (staged changes)")
    parser.add_argument("--pr", type=str, help="Analyze PR diff against base branch (e.g., 'main')")
    parser.add_argument("--prompt", type=str, default=DEFAULT_PROMPT,
                        help="Custom analysis prompt")
    parser.add_argument("--no-include", action="store_true",
                        help="Disable automatic header inclusion (raw diff/files only)")
    return parser.parse_args()


def resolve_include_path(include: str, source_file: str) -> str | None:
    source_dir = os.path.dirname(os.path.abspath(source_file))
    candidate = os.path.normpath(os.path.join(source_dir, include))
    if os.path.exists(candidate):
        return candidate
    return None


def get_local_includes(file_path: str, resolved: Set[str], depth: int = INCLUDE_DEPTH_LIMIT) -> None:
    if depth <= 0:
        return

    abs_path = os.path.abspath(file_path)
    if abs_path in resolved or not os.path.exists(abs_path):
        return

    resolved.add(abs_path)

    try:
        with open(abs_path, 'r') as f:
            content = f.read()
    except Exception:
        return

    # Only do include with "" not <>, cause we don't want system headers,
    # just Sync specific includes
    local_includes = re.findall(r'#include\s+"([^"]+)"', content)

    for inc in local_includes:
        if os.path.basename(inc) in EXCLUDED_HEADERS:
            continue

        inc_path = resolve_include_path(inc, abs_path)
        if inc_path and inc_path not in resolved:
            get_local_includes(inc_path, resolved, depth - 1)


def gather_context_files(source_files: List[str]) -> tuple[Set[str], Set[str]]:
    primary = set()
    all_resolved = set()

    for f in source_files:
        if os.path.exists(f) and f.endswith(('.c', '.cpp', '.h', '.hpp')):
            abs_path = os.path.abspath(f)
            primary.add(abs_path)
            get_local_includes(f, all_resolved, INCLUDE_DEPTH_LIMIT)

    reference = all_resolved - primary

    return primary, reference


def extract_declarations(content: str) -> str:
    lines = content.split('\n')
    result = []
    brace_depth = 0
    in_function_body = False

    for line in lines:
        stripped = line.strip()

        if stripped.startswith('#') or stripped.startswith('//') or not stripped:
            result.append(line)
            continue

        open_braces = line.count('{')
        close_braces = line.count('}')

        # detect function body start (has { but not struct/class/enum/namespace)
        if open_braces > 0 and brace_depth == 0:
            is_type_def = any(kw in stripped for kw in ['struct', 'class', 'enum', 'namespace', 'union'])
            if not is_type_def and '(' in stripped:
                in_function_body = True

        if not in_function_body or brace_depth == 0:
            if in_function_body and '{' in line:
                sig_part = line.split('{')[0].strip()
                if sig_part:
                    result.append(sig_part + ';  // body omitted')
            else:
                result.append(line)

        brace_depth += open_braces - close_braces

        if in_function_body and brace_depth == 0:
            in_function_body = False

    return '\n'.join(result)


def read_file_contents(file_paths: Set[str], section_label: str = "", compress: bool = False, line_numbers: bool = True) -> str:
    content_parts = []
    for path in sorted(file_paths):
        try:
            rel_path = os.path.relpath(path)
        except ValueError:
            rel_path = path

        try:
            with open(path, 'r') as f:
                raw_content = f.read()

            if compress:
                raw_content = extract_declarations(raw_content)

            if line_numbers:
                lines = raw_content.split('\n')
                numbered_lines = []
                for i, line in enumerate(lines, 1):
                    numbered_lines.append(f"{rel_path}:{i}: {line.rstrip()}")
                content = "\n".join(numbered_lines)
            else:
                content = raw_content

            marker = " (context only)" if compress else ""
            content_parts.append(f"=== FILE: {rel_path}{marker} ===\n{content}\n=== END: {rel_path} ===")
        except Exception as e:
            print(f"Warning: Could not read {rel_path}: {e}")

    if not content_parts:
        return ""

    body = "\n\n".join(content_parts)
    if section_label:
        return f"=== {section_label} ({len(file_paths)} files) ===\n\n{body}"
    return body


def format_context(primary: Set[str], reference: Set[str], diff: str = "") -> str:
    """LLM recency bias so put important stuff later"""
    parts = []

    if reference:
        parts.append(read_file_contents(
            reference, "REFERENCE HEADERS",
            compress=COMPRESS_REFERENCE_HEADERS,
            line_numbers=False
        ))

    if primary:
        parts.append(read_file_contents(
            primary, "PRIMARY FILES",
            compress=False,
            line_numbers=True
        ))

    if diff.strip():
        parts.append(f"=== DIFF ===\n{diff}")

    return "\n\n".join(parts)


def read_files(file_paths: List[str], include_context: bool = True) -> str:
    if include_context:
        primary, reference = gather_context_files(file_paths)
        return format_context(primary, reference, diff="")

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


def get_changed_files_from_diff(staged_only: bool = True) -> List[str]:
    cmd = ["git", "diff", "--name-only", "--cached"] if staged_only else ["git", "diff", "--name-only"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return [f for f in result.stdout.strip().split('\n') if f]


def get_git_diff(staged_only: bool = True, include_context: bool = True) -> str:
    cmd = ["git", "diff", "--cached"] if staged_only else ["git", "diff"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    diff = result.stdout

    if not include_context or not diff.strip():
        return diff

    changed_files = get_changed_files_from_diff(staged_only)
    primary, reference = gather_context_files(changed_files)

    return format_context(primary, reference, diff)


def get_changed_files_from_pr(base_branch: str) -> List[str]:
    merge_base = subprocess.run(
        ["git", "merge-base", base_branch, "HEAD"],
        capture_output=True, text=True
    ).stdout.strip()

    result = subprocess.run(
        ["git", "diff", "--name-only", merge_base, "HEAD"],
        capture_output=True, text=True
    )
    return [f for f in result.stdout.strip().split('\n') if f]


def get_pr_diff(base_branch: str = "main", include_context: bool = True) -> str:
    merge_base = subprocess.run(
        ["git", "merge-base", base_branch, "HEAD"],
        capture_output=True, text=True
    ).stdout.strip()

    result = subprocess.run(
        ["git", "diff", merge_base, "HEAD"],
        capture_output=True, text=True
    )
    diff = result.stdout

    if not include_context or not diff.strip():
        return diff

    changed_files = get_changed_files_from_pr(base_branch)
    primary, reference = gather_context_files(changed_files)

    return format_context(primary, reference, diff)


def query_ollama_streaming(model: str, prompt: str, context: str):
    # context first, instructions last
    # recency bias helps smaller models
    full_prompt = f"{context}\n\n{prompt}\n{OUTPUT_PRIMER}"

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
    include_context = not args.no_include

    context = ""

    if args.files:
        context = read_files(args.files, include_context=include_context)
    elif args.pr:
        context = get_pr_diff(args.pr, include_context=include_context)
        if not context.strip() or context.strip() == f"=== CONTEXT FILES (0 files) ===\n\n\n\n=== DIFF ===\n":
            print(f"No diff found between {args.pr} and HEAD")
            exit(1)
    elif args.diff:
        context = get_git_diff(staged_only=True, include_context=include_context)
        if not context.strip() or "=== DIFF ===\n" in context and context.endswith("=== DIFF ===\n"):
            context = get_git_diff(staged_only=False, include_context=include_context)
        if not context.strip():
            print("No changes to analyze")
            exit(1)
    else:
        print("Specify --files, --diff, or --pr to provide code for analysis")
        exit(1)

    print(f"Using model: {args.model}")
    print(f"Context gathering: {'disabled' if args.no_include else 'enabled (depth=' + str(INCLUDE_DEPTH_LIMIT) + ')'}")
    print(f"Analyzing {len(context)} characters of content...\n")

    query_ollama_streaming(args.model, args.prompt, context)
