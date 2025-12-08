import os
from pathlib import Path
import subprocess
from typing import List

filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

try:
    result = subprocess.run("ctest --test-dir build -N", capture_output=True, text=True, check=True)
    lines: List[str] = result.stdout.split("\n")
    start_index = 0
    end_index = 0
    for i, line in enumerate(lines):
        line = line.lstrip() # remove leading whitespace
        if line.startswith("Internal ctest") or line.startswith("Test project"):
            start_index += 1
            continue

        if line == "" or line.startswith("Total Tests:"):
            end_index = i
            break

    test_names: List[str]= []
    for line in lines[start_index:end_index]:
        name = line.split(":")[1].lstrip()
        if "Death" in name: # TODO death test coverage
            continue
        test_names.append(name)

    combine_command: str = "OpenCppCoverage.exe"
    for name in test_names:
        combine_command += f" --input_coverage build\\test\\{name}.cov"
    combine_command += " --export_type html:CombinedCoverage"
    print(combine_command)

    subprocess.run(combine_command, check=True)
    
except subprocess.CalledProcessError as e:
    print(f"Command failed with exit code {e.returncode}")
    print(f"Error output: {e.stderr}")