# Copyright (c) 2019-2023 Lukasz Stalmirski
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
VULKAN_HEADERS_DIR = os.path.abspath( sys.argv[ 1 ] )
CMAKE_CURRENT_BINARY_DIR = os.path.abspath( sys.argv[ 2 ] )

class VulkanSpec:
    def __init__( self, vk_xml: etree.ElementTree ):
        self.xml = vk_xml.getroot()
        self.types = self.xml.find( "types" )
        self.commands = self.xml.find( "commands" )
        self.extensions = self.xml.find( "extensions" )
        self.vulkansc = self.xml.find( "feature[@api=\"vulkansc\"]" )
        self.handles = [handle.text for handle in self.types.findall( "type[@category=\"handle\"][type=\"VK_DEFINE_HANDLE\"]/name" )]

    def get_command_name( self, cmd: etree.Element ):
        try: # Function definition
            return cmd.find( "proto/name" ).text
        except: # Function alias
            return cmd.get( "name" )

    def get_command_handle( self, cmd: etree.Element ):
        try: # Function definition
            return cmd.find( "param[1]/type" ).text
        except: # Function alias
            return self.commands.find( "command/proto[name=\"" + cmd.get( "alias" ) + "\"]/../param[1]/type" ).text

    def get_command_extension( self, cmd: etree.Element ):
        try: # Extension function
            cmd_name = self.get_command_name( cmd )
            return self.extensions.find( "extension/require/command[@name=\"" + cmd_name + "\"]/../.." ).get( "name" )
        except AttributeError: # Core function
            return None

    def is_vulkansc_command( self, cmd: etree.Element ):
        # Check if cmd is secure version of a standard functions
        if "api" in cmd.attrib.keys() and cmd.attrib[ "api" ] == "vulkansc":
            return True
        # Check if cmd is required by vulkansc feature set
        cmd_name = self.get_command_name( cmd )
        if self.vulkansc is not None and self.vulkansc.find( "require/command[@name=\"" + cmd_name + "\"]" ) is not None:
            return True
        return False

class DispatchTableCommand:
    def __init__( self, name, extension ):
        self.name = name[2:]
        self.extension = extension

# Dispatch tables
class DispatchTableGenerator:
    def __init__( self, vk_xml: etree.ElementTree ):
        self.instance_dispatch_table = {}
        self.device_dispatch_table = {}
        spec = VulkanSpec( vk_xml )
        for cmd in spec.commands.findall( "command" ):
            if not spec.is_vulkansc_command( cmd ):
                cmd_handle = spec.get_command_handle( cmd )
                cmd_name = spec.get_command_name( cmd )
                cmd_extension = spec.get_command_extension( cmd )
                command = DispatchTableCommand( cmd_name, cmd_extension )
                # Check which chain the command belongs to
                if cmd_handle in ("VkInstance", "VkPhysicalDevice") or (cmd_handle not in spec.handles):
                    self.insert_or_append( self.instance_dispatch_table, command )
                else:
                    self.insert_or_append( self.device_dispatch_table, command )

    def insert_or_append( self, dst: dict, command: DispatchTableCommand ):
        if command.extension in dst.keys():
            dst[ command.extension ].append( command )
        else:
            dst[ command.extension ] = [ command ]

    def write_commands( self, out: io.TextIOBase ):
        out.write( "#pragma once\n" )
        out.write( "#include \"" + os.path.join( VULKAN_HEADERS_DIR, "include", "vulkan", "vk_layer.h" ) + "\"\n\n" )
        out.write( "typedef struct VkLayerInstanceDispatchTable {\n" )
        for extension, commands in self.instance_dispatch_table.items():
            self.write_extension_commands( out, extension, commands, "  PFN_vk{0} {0};\n" )
        out.write( "} VkLayerInstanceDispatchTable;\n\n" )
        out.write( "typedef struct VkLayerDispatchTable {\n" )
        for extension, commands in self.device_dispatch_table.items():
            self.write_extension_commands( out, extension, commands, "  PFN_vk{0} {0};\n" )
        out.write( "} VkLayerDeviceDispatchTable;\n\n" )
        out.write( "inline void init_layer_instance_dispatch_table( VkInstance instance, PFN_vkGetInstanceProcAddr gpa, VkLayerInstanceDispatchTable& dt ) {\n" )
        for extension, commands in self.instance_dispatch_table.items():
            self.write_extension_commands( out, extension, commands, "  dt.{0} = (PFN_vk{0}) gpa( instance, \"vk{0}\" );\n" )
        out.write( "}\n\n" )
        out.write( "inline void init_layer_device_dispatch_table( VkDevice device, PFN_vkGetDeviceProcAddr gpa, VkLayerDeviceDispatchTable& dt ) {\n" )
        for extension, commands in self.device_dispatch_table.items():
            self.write_extension_commands( out, extension, commands, "  dt.{0} = (PFN_vk{0}) gpa( device, \"vk{0}\" );\n" )
        out.write( "}\n" )

    def write_extension_commands( self, out: io.TextIOBase, extension: str, commands: list, format: str ):
        if extension is not None:
            out.write( "  #ifdef " + extension + "\n" )
        for cmd in commands:
            out.write( str.format( format, cmd.name ) )
        if extension is not None:
            out.write( "  #endif\n" )

# Generate dispatch tables
def gen_dispatch_tables():
    out_path = os.path.join( CMAKE_CURRENT_BINARY_DIR, "vk_dispatch_tables.h" )
    vk_xml_path = os.path.join( VULKAN_HEADERS_DIR, "registry", "vk.xml" )
    vk_xml = etree.parse( vk_xml_path )
    generator = DispatchTableGenerator( vk_xml )
    with open( out_path, mode="w" ) as out:
        generator.write_commands( out )

# Main
if __name__ == "__main__":
    gen_dispatch_tables()
