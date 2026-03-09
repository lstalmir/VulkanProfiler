# Copyright (c) 2023-2026 Lukasz Stalmirski
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys
import subprocess
import datetime

def git(*args):
  proc = subprocess.run(["git", *args], stdout=subprocess.PIPE)
  return proc.stdout.decode().strip()

try:
  remote = git("rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}")
  remote_sep = remote.index('/')
  remote = remote[:remote_sep]
except Exception as e:
  remote = "origin"

# Get git repository url
git_url = git("remote", "get-url", remote)

# Get the commit this branch was based on
rev_base = git("merge-base", f"{remote}/master", "HEAD")

# Get number of commits in the base branch
rev_base_count = int(git("rev-list", "--count", rev_base))

# Get number of commits made on this branch
rev_count = int(git("rev-list", "--count", "HEAD"))

# Get the base commit date in strict ISO format
result = git("show", "-s", "--format=%cI", rev_base)
rev_base_date = datetime.datetime.fromisoformat(result)

# Write cmake file with the version info
with open(sys.argv[1], "w") as output:
  output.write(f"set (PROFILER_LAYER_VER_MAJOR {rev_base_date.year})\n")
  output.write(f"set (PROFILER_LAYER_VER_MINOR {rev_base_date.month})\n")
  output.write(f"set (PROFILER_LAYER_VER_BUILD {rev_base_count})\n")
  output.write(f"set (PROFILER_LAYER_VER_PATCH {rev_count - rev_base_count})\n")
  output.write(f"set (PROFILER_LAYER_GIT_URL {git_url})\n")
