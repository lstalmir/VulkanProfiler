# Copyright (c) 2023 Lukasz Stalmirski
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

# Get the commit this branch was based on
rev_base = git("merge-base", "origin/master", "HEAD")

# Get number of commits in the base branch
rev_base_count = int(git("rev-list", "--count", rev_base))

# Get number of commits made on this branch
rev_count = int(git("rev-list", "--count", "HEAD"))

# Get the base commit date in strict ISO format
result = git("show", "-s", "--format=%cI", rev_base)
rev_base_date = datetime.datetime.fromisoformat(result)

ver_major = rev_base_date.year
ver_minor = rev_base_date.month
ver_build = rev_base_count
ver_patch = rev_count - rev_base_count

# Write cmake file with the version info
with open(sys.argv[1], "w") as output:
  profiler_version_cmake = \
    "set (PROFILER_LAYER_VER_MAJOR {})\n".format(ver_major) + \
    "set (PROFILER_LAYER_VER_MINOR {})\n".format(ver_minor) + \
    "set (PROFILER_LAYER_VER_BUILD {})\n".format(ver_build) + \
    "set (PROFILER_LAYER_VER_PATCH {})\n".format(ver_patch)
  output.write(profiler_version_cmake)
