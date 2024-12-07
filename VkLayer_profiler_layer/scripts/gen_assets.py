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
import json
import types

class Resources:
    def __init__( self, input_path ):
        with open( input_path ) as f:
            self.resources = json.load( f, object_hook=lambda d: types.SimpleNamespace( **d ) )
        self.basepath = os.path.dirname( input_path )

    def write_header_resource( self, resource, out ):
        with open( os.path.join( self.basepath, resource.path ), "rb" ) as res:
            out.write( f"static const uint8_t {resource.name}[] = {{ " )
            data = []
            for byte in res.read():
                data.append( f"0x{byte:02x}" )
            out.write( ",".join( data ) )
            out.write( " };\n" )

    def write_header( self, output_path, group_name ):
        with open( output_path, "w" ) as out:
            out.write( "#pragma once\n" )
            out.write( "#include <stdint.h>\n\n" )
            out.write( f"namespace Profiler::{group_name}\n{{\n" )
            for resource in self.resources:
                self.write_header_resource( resource, out )
            out.write( "}\n" )

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument( "input", type=str )
    parser.add_argument( "--output", "-o", type=str, dest="output", required=True )
    parser.add_argument( "--group", type=str, dest="group", default="Resources", required=False )
    args = parser.parse_args()

    resources = Resources( args.input )
    resources.write_header( args.output, args.group )
