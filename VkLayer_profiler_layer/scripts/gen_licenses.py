# Copyright (c) 2024 Lukasz Stalmirski
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

# List of file names (without extension) to scan for licenses.
LICENSE_FILES = [ "COPYING", "LICENSE" ]

def find_licenses( directory ):
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
            licenses[ lib ] = path
            break
        if lib not in licenses.keys():
            raise Exception( f"License not found for {lib}" )
    return licenses

def append_license_file( out, name, path ):
    out.write( f"[{name}]\n" )
    with open( path ) as license_file:
        out.write( license_file.read() )
    out.write( "\n" )

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument( "--license", action="append", nargs=2, dest="license_paths", metavar=("NAME", "PATH") )
    parser.add_argument( "--externaldir", action="append", dest="external_dirs" )
    parser.add_argument( "--output", "-o", dest="output", required=True )
    args = parser.parse_args()

    with open( args.output, mode="w" ) as out:
        for lib, path in args.license_paths:
            append_license_file( out, lib, path )
        for directory in args.external_dirs:
            licenses = find_licenses( directory )
            for lib, path in licenses.items():
                append_license_file( out, lib, path )

    print( f"-- Licenses written to {args.output}" )
