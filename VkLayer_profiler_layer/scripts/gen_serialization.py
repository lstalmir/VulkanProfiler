# Copyright (c) 2025 Lukasz Stalmirski
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

from re import M
import sys
import os
import xml.etree.ElementTree as etree

# Read configuration variables
VULKAN_HEADERS_DIR = os.path.abspath( sys.argv[ 1 ] )
CMAKE_CURRENT_BINARY_DIR = os.path.abspath( sys.argv[ 2 ] )

# Parse Vulkan specification and extract structures
out_path = os.path.join( CMAKE_CURRENT_BINARY_DIR, 'vk_serialization.h' )
vk_xml_path = os.path.join( VULKAN_HEADERS_DIR, 'registry', 'vk.xml' )
vk_xml = etree.parse( vk_xml_path )
vk_structures = vk_xml.findall( 'types/type[@category="struct"]' )

# Group structures by extension
vk_structure_exts = {}
vk_structure_sc = []
for ext in vk_xml.findall( 'extensions/extension' ):
    ext_name = ext.get( 'name' )
    for ext_require in ext.findall( 'require' ):
        ext_require_sc = (ext_require.get( 'api' ) == 'vulkansc')
        for ext_type in ext_require.findall( 'type' ):
            ext_typename = ext_type.get( 'name' )
            vk_structure_exts[ext_typename] = ext_name
            if ext_require_sc:
                vk_structure_sc.append( ext_typename )

for feature in vk_xml.findall( 'feature[@api="vulkansc"]' ):
    for feature_type in feature.findall( 'require/type' ):
        feature_typename = feature_type.get( 'name' )
        vk_structure_sc.append( feature_typename )

# Returns name of the member that contains length of the array, or None if the member is not an array
def get_member_len( member, all_members ):
    member_len = member.get( 'altlen' )
    if member_len:
        for other_member in all_members:
            other_member_name = other_member.find( 'name' ).text
            member_len = member_len.replace( other_member_name, 'value.' + other_member_name )
        return member_len
    else:
        member_len = member.get( 'len' )
    return member_len

# Write structure serialization and deserilization functions
with open( out_path, 'w' ) as out:
    out.write( '#pragma once\n' )
    out.write( '#include <vulkan/vulkan.h>\n\n' )

    for struct in vk_structures:
        if struct.get( 'alias' ) is not None:
            continue

        name = struct.get( 'name' )
        if name in vk_structure_sc:
            continue

        ext = vk_structure_exts.get( name, None )
        if ext is not None:
            out.write( f'#ifdef {ext}\n' )

        out.write( 'template<typename Stream>\n' )
        out.write( f'inline Stream& operator<<( Stream& stream, const {name}& value ) {{\n' )

        members = struct.findall( 'member' )
        for member in members:
            member_name = member.find( 'name' ).text
            if member_name == 'pNext':
                continue
            if member.get( 'api' ) == 'vulkansc':
                continue
            member_len = get_member_len( member, members )
            if member_len == 'null-terminated':
                out.write( f'  stream << value.{member_name};\n' )
            elif member_len:
                out.write( f'  stream << static_cast<uint32_t>({member_len});\n' )
                out.write( f'  for( uint32_t i = 0; i < static_cast<uint32_t>({member_len}); ++i ) {{\n' )
                out.write( f'    stream << value.{member_name}[ i ];\n' )
                out.write(  '  }\n' )
            else:
                out.write( f'  stream << value.{member_name};\n' )
        out.write( '  return stream;\n' )
        out.write( '}\n' )

        if ext is not None:
            out.write( '#endif\n' )
        out.write( '\n' )
