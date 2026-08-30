# Copyright (c) 2026 Lukasz Stalmirski
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
import io
import xml.etree.ElementTree as etree

# Read configuration variables
STRING_MAPPINGS_XML_PATH = os.path.abspath( sys.argv[ 1 ] )
STRING_MAPPINGS_OUTPUT_PATH = os.path.abspath( sys.argv[ 2 ] )

# Dispatch tables
class StringMappingsGenerator:
    def __init__( self, mappings_xml: etree.ElementTree ):
        self.includes = mappings_xml.getroot().findall( "Include" )
        self.mappings = mappings_xml.getroot().findall( "Map" )

    def write_mappings( self, out: io.TextIOBase ):
        out.write( "#pragma once\n" )

        for include in self.includes:
            include_path = include.attrib[ "Path" ]
            out.write( f"#include <{include_path}>\n" )
        out.write( "#include <utils/fnv.h>\n" )
        out.write( "\n" )

        out.write( "namespace Profiler {\n\n" )

        for mapping in self.mappings:
            self.write_map( out, mapping )

        out.write( "} // namespace Profiler\n" )

    def write_map( self, out: io.TextIOBase, map: etree.Element ):
        map_name = map.attrib[ "Name" ]
        map_key_type = map.attrib[ "KeyType" ]
        map_entries = map.findall( "Entry" )

        out.write( "inline constexpr struct {\n" )
        out.write( "  typedef " + map_key_type + " KeyType;\n" )
        self.write_map_apply( out, map )
        self.write_map_forward_getter( out, map )
        self.write_map_backward_getter( out, map )
        out.write( f"}} {map_name};\n\n" )

    def write_map_apply( self, out: io.TextIOBase, map: etree.Element ):
        map_name = map.attrib[ "Name" ]
        map_key_type = map.attrib[ "KeyType" ]
        map_entries = map.findall( "Entry" )

        out.write( "  template<typename Fn>\n" )
        out.write( "  inline constexpr void Apply( Fn&& fn ) const {\n" )
        for entry in map_entries:
            entry_name = entry.attrib[ "Key" ]
            entry_value = entry.attrib[ "Value" ]
            out.write( f"    fn( {entry_name}, \"{entry_value}\" );\n" )
        out.write( "  }\n" )

    def write_map_forward_getter( self, out: io.TextIOBase, map: etree.Element ):
        map_name = map.attrib[ "Name" ]
        map_key_type = map.attrib[ "KeyType" ]
        map_entries = map.findall( "Entry" )

        out.write( f"  inline constexpr std::string_view operator[]( {map_key_type} key ) const {{\n" )
        out.write( "    switch( key ) {\n" )
        for entry in map_entries:
            entry_name = entry.attrib[ "Key" ]
            entry_value = entry.attrib[ "Value" ]
            out.write( f"      case {entry_name}: return \"{entry_value}\";\n" )
        out.write( "    }\n" )
        out.write( "    return \"\";\n" )
        out.write( "  }\n" )

    def write_map_backward_getter( self, out: io.TextIOBase, map: etree.Element ):
        map_name = map.attrib[ "Name" ]
        map_key_type = map.attrib[ "KeyType" ]
        map_entries = map.findall( "Entry" )

        out.write( f"  inline constexpr {map_key_type} operator[]( std::string_view value ) const {{\n" )
        out.write( f"    switch( FNV( value ) ) {{\n" )
        for entry in map_entries:
            entry_name = entry.attrib[ "Key" ]
            entry_value = entry.attrib[ "Value" ]
            out.write( f"      case FNV( \"{entry_value}\" ): return {entry_name};\n" )
        out.write( "    }\n" )
        out.write( "    return static_cast<" + map_key_type + ">( -1 );\n" )
        out.write( "  }\n" )

    def get_c_identifier( self, name: str ) -> str:
        return name.split( '::' )[ -1 ]

# Generate string mappings
def gen_string_mappings():
    mappings_xml = etree.parse( STRING_MAPPINGS_XML_PATH )
    generator = StringMappingsGenerator( mappings_xml )
    with open( STRING_MAPPINGS_OUTPUT_PATH, mode="w" ) as out:
        generator.write_mappings( out )

if __name__ == "__main__":
    gen_string_mappings()
