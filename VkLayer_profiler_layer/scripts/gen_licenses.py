# Copyright (c) 2024-2026 Lukasz Stalmirski
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
import os
import pathlib
import argparse
import subprocess
import locale

# List of file names (without extension) to scan for licenses.
LICENSE_FILES = [ "COPYING", "LICENSE" ]

# Get the default encoding to use for reading license files.
DEFAULT_ENCODING = locale.getpreferredencoding()
SUPPORTED_ENCODINGS = [ DEFAULT_ENCODING, "utf-8" ]

class library_info:
    def __init__( self, name: str, license_path: str ):
        self.name = name
        self.license_path = license_path
        self.git_url = None

def execute_process( cmd, cwd ):
    result = subprocess.check_output( cmd, cwd=cwd, stderr=subprocess.DEVNULL )
    return result.decode( "utf-8" ).strip()

def find_licenses( directory, skip_libs ):
    licenses = {}
    for lib in os.listdir( directory ):
        libdir = os.path.join( directory, lib )
        if not os.path.isdir( libdir ):
            continue
        for file in os.listdir( libdir ):
            path = os.path.join( libdir, file )
            if not os.path.isfile( path ):
                continue
            filename = pathlib.Path( path ).stem
            if not filename in LICENSE_FILES:
                continue
            info = library_info( lib, path )
            if os.path.exists( os.path.join( libdir, ".git" ) ):
                try:
                    remote = execute_process( ["git", "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{u}"], cwd=libdir )
                    remote_sep = remote.index('/')
                    remote = remote[:remote_sep]
                except Exception as e:
                    remote = "origin"
                info.git_url = execute_process( ["git", "remote", "get-url", remote], cwd=libdir )
            licenses[ lib ] = info
            break
        if lib not in licenses.keys() and lib not in skip_libs:
            raise Exception( f"ERROR: License not found for {lib}" )
    return licenses.values()

def read_license_file( license_path ):
    for encoding in SUPPORTED_ENCODINGS:
        try:
            with open( license_path, encoding=encoding ) as license_file:
                return license_file.readlines()
        except UnicodeDecodeError:
            pass
    raise Exception( f"Failed to read '{license_path}' using any of the known encodings" )

def append_license_file( out, info: library_info ):
    out.write( f"| **{info.name}**\n" )
    if info.git_url:
        out.write( f"| {info.git_url}\n" )
    out.write( "\n" )
    out.write( ".. code-block:: text\n\n" )
    for line in read_license_file( info.license_path ):
        out.write( "  " + line.strip() + "\n" )
    out.write( "\n\n" )

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument( "--license", action="append", nargs=2, dest="license_paths", metavar=("NAME", "PATH"), default=[] )
    parser.add_argument( "--skip", nargs="*", dest="skip_libs", metavar=("NAME"), default=[] )
    parser.add_argument( "--externaldir", action="append", dest="external_dirs" )
    parser.add_argument( "--output", "-o", dest="output", required=True )
    args = parser.parse_args()

    with open( args.output, mode="w" ) as out:
        for lib, path in args.license_paths:
            info = library_info( lib, path )
            append_license_file( out, info )
        for directory in args.external_dirs:
            licenses = find_licenses( directory, args.skip_libs )
            for info in licenses:
                append_license_file( out, info )

    print( f"-- Licenses written to {args.output}" )
