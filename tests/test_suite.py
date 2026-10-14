#!/usr/bin/env python3
import subprocess
import os
import sys
import tempfile
import shutil

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SHELL_BIN = os.path.join(PROJECT_ROOT, "shell")

class TestRunner:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.tests = []

    def run_shell(self, commands, timeout=5):
        """Run shell with commands passed via stdin"""
        cmd_str = "\n".join(commands) + "\nexit\n"
        proc = subprocess.Popen(
            [SHELL_BIN],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=PROJECT_ROOT
        )

        try:
            stdout, stderr = proc.communicate(input=cmd_str, timeout=timeout)
            return stdout, stderr, proc.returncode
        except subprocess.TimeoutExpired:
            proc.kill()
            return "", "Timeout", -1

    def test(self, name, commands, validator):
        print(f"Running test: {name:<42} ... ", end="", flush=True)
        stdout, stderr, code = self.run_shell(commands)
        try:
            ok, msg = validator(stdout, stderr, code)
            if ok:
                print("\033[92mPASSED\033[0m")
                self.passed += 1
            else:
                print(f"\033[91mFAILED\033[0m: {msg}")
                self.failed += 1
        except Exception as e:
            print(f"\033[91mEXCEPTION\033[0m: {e}")
            self.failed += 1

runner = TestRunner()

# Test 1: Echo and Pwd
def test_echo_pwd(stdout, stderr, code):
    if "Hello Embedded World" in stdout and os.getcwd() in stdout:
        return True, ""
    return False, f"Output did not match. stdout:\n{stdout}"
runner.test("Echo and Pwd", ["echo Hello Embedded World", "pwd"], test_echo_pwd)

# Test 2: Quotes in Echo
def test_quotes(stdout, stderr, code):
    if "hello world; from 'quotes'" in stdout:
        return True, ""
    return False, f"Expected quoted string preserved. stdout:\n{stdout}"
runner.test("Echo Quoted String", ["echo \"hello world; from 'quotes'\""], test_quotes)

# Test 3: Multiple Commands separated by Semicolon
def test_semicolon(stdout, stderr, code):
    lines = [l.strip() for l in stdout.splitlines() if l.strip()]
    if any("first_cmd" in l for l in lines) and any("second_cmd" in l for l in lines) and any("third_cmd" in l for l in lines):
        return True, ""
    return False, f"Semicolon sequence failed. stdout:\n{stdout}"
runner.test("Semicolon Sequencing", ["echo first_cmd; echo second_cmd; echo third_cmd"], test_semicolon)

# Test 4: cd and cd -
def test_cd(stdout, stderr, code):
    lines = [l.strip() for l in stdout.splitlines() if l.strip()]
    cwd = os.getcwd()
    if any(cwd in l for l in lines):
        return True, ""
    return False, f"cd - failed to return to previous directory. stdout:\n{stdout}"
runner.test("cd, cd .., cd -", ["cd ..", "pwd", "cd -", "pwd"], test_cd)

# Test 5: ls and ls -l
def test_ls(stdout, stderr, code):
    if "Makefile" in stdout and "README.md" in stdout and "total" in stdout:
        return True, ""
    return False, f"ls -la failed to list files or total. stdout:\n{stdout}"
runner.test("ls -la current dir", ["ls -la"], test_ls)

# Test 6: ls on a single file
def test_ls_file(stdout, stderr, code):
    if "Makefile" in stdout and "cannot access" not in stderr and "Not a directory" not in stderr:
        return True, ""
    return False, f"ls single file failed. stdout:\n{stdout}\nstderr:\n{stderr}"
runner.test("ls single file (ls -l Makefile)", ["ls -l Makefile"], test_ls_file)

# Test 7: Output and Input Redirection
def test_redirection(stdout, stderr, code):
    test_file = "test_redir_temp.txt"
    if os.path.exists(test_file): os.remove(test_file)
    stdout, stderr, code = runner.run_shell([
        f'echo "systems programming test" > {test_file}',
        f'cat < {test_file}',
        f'echo "second line" >> {test_file}',
        f'cat < {test_file}'
    ])
    if os.path.exists(test_file): os.remove(test_file)
    if "systems programming test" in stdout and "second line" in stdout:
        return True, ""
    return False, f"Redirection test failed. stdout:\n{stdout}"
runner.test("I/O Redirection (>, <, >>)", [], lambda *args: test_redirection(*args))

# Test 8: Pipelines with system commands
def test_pipeline(stdout, stderr, code):
    lines = [l.strip() for l in stdout.splitlines() if l.strip()]
    if any(l == "Makefile" for l in lines):
        return True, ""
    return False, f"Pipeline failed. stdout:\n{stdout}"
runner.test("Pipeline (ls | grep Makefile)", ["ls | grep Makefile"], test_pipeline)

# Test 9: Builtins inside pipeline
def test_builtin_pipeline(stdout, stderr, code):
    lines = [l.strip() for l in stdout.splitlines() if l.strip()]
    if any("4" in l for l in lines):
        return True, ""
    return False, f"Builtin pipeline failed. stdout:\n{stdout}"
runner.test("Builtin in pipeline (echo ... | wc -w)", ['echo "hello world foo bar" | wc -w'], test_builtin_pipeline)

# Test 10: Multi-stage pipeline with redirection
def test_pipeline_redirection(stdout, stderr, code):
    out_file = "test_pipe_redir_temp.txt"
    if os.path.exists(out_file): os.remove(out_file)
    stdout, stderr, code = runner.run_shell([
        f'ls | grep Makefile > {out_file}',
        f'cat < {out_file}'
    ])
    content = ""
    if os.path.exists(out_file):
        with open(out_file) as f: content = f.read()
        os.remove(out_file)
    if "Makefile" in content:
        return True, ""
    return False, f"Pipeline redirection failed. stdout:\n{stdout}, file content:\n{content}"
runner.test("Pipeline with Redirection", [], lambda *args: test_pipeline_redirection(*args))

# Test 11: search builtin
def test_search(stdout, stderr, code):
    lines = [l.strip() for l in stdout.splitlines() if l.strip()]
    if "True" in lines and "False" in lines:
        return True, ""
    return False, f"Search failed. stdout:\n{stdout}"
runner.test("Search (existing and non-existing)", ["search Makefile", "search nonexistent_random_file_xyz"], test_search)

# Test 12: pinfo builtin
def test_pinfo(stdout, stderr, code):
    if "Process Status --" in stdout and "memory --" in stdout and "Executable Path --" in stdout:
        return True, ""
    return False, f"pinfo output format mismatch. stdout:\n{stdout}"
runner.test("pinfo self", ["pinfo"], test_pinfo)

# Test 13: history builtin
def test_history(stdout, stderr, code):
    if "history" in stdout:
        return True, ""
    return False, f"history failed. stdout:\n{stdout}"
runner.test("History command", ["echo hist1", "echo hist2", "history 5"], test_history)

# Test 14: Background Execution (&)
def test_background(stdout, stderr, code):
    if "[" in stdout and "]" in stdout:
        return True, ""
    return False, f"Background PID not printed. stdout:\n{stdout}"
runner.test("Background process PID ([pid])", ["sleep 0.1 &"], test_background)

# Test 15: Submodule Path Navigation & Recursive Search
def test_submodule_paths(stdout, stderr, code):
    if "src/builtins" in stdout and stdout.count("True") >= 2:
        return True, ""
    return False, f"Submodule navigation or recursive search failed. stdout:\n{stdout}"
runner.test("Submodule Navigation & Search", ["cd src/builtins", "pwd", "search Builtins.cpp", "cd -", "search Shell.h"], test_submodule_paths)

# Test 16: Submodule Directory Listing
def test_submodule_ls(stdout, stderr, code):
    if "Shell.cpp" in stdout and "Signals.cpp" in stdout and "main.cpp" in stdout:
        return True, ""
    return False, f"Submodule ls failed. stdout:\n{stdout}"
runner.test("Submodule Directory Listing (ls -l src/core)", ["ls -l src/core"], test_submodule_ls)

print("=" * 60)
print(f"Final Results: {runner.passed} Passed, {runner.failed} Failed")
print("=" * 60)
sys.exit(0 if runner.failed == 0 else 1)
