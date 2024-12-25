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

import xml.etree.ElementTree as etree

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

    def get_command_result( self, cmd: etree.Element ):
        try: # Function definition
            return cmd.find( "proto/type" ).text
        except: # Function alias
            return self.commands.find( "command/proto[name=\"" + cmd.get( "alias" ) + "\"]/../proto/type" ).text

    def get_command_handle( self, cmd: etree.Element ):
        handle = None
        try: # Function definition
            handle = cmd.find( "param[1]/type" ).text
        except: # Function alias
            handle = self.commands.find( "command/proto[name=\"" + cmd.get( "alias" ) + "\"]/../param[1]/type" ).text
        if handle in self.handles:
            return handle
        return None

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

class VulkanCommandParam:
    def __init__( self, spec: VulkanSpec, param: etree.Element ):
        self.string = ''.join( param.itertext() )
        self.name = param.find( 'name' ).text
        self.vulkansc = False
        if param.attrib.get( 'api' ) is not None:
            self.vulkansc = ( param.attrib[ 'api' ] == 'vulkansc' )

    def __repr__( self ):
        return self.string

    def __str__( self ):
        return self.string

class VulkanCommand:
    def __init__( self, spec: VulkanSpec, cmd: etree.Element ):
        self.name = spec.get_command_name( cmd )
        self.alias = cmd.get( 'alias' )
        if self.alias is not None:
            cmd = spec.commands.find( "command/proto[name=\"" + self.alias + "\"]/.." )
        self.result = spec.get_command_result( cmd )
        self.params = [VulkanCommandParam( spec, param ) for param in cmd.findall( 'param' )]
        self.handle = spec.get_command_handle( cmd )
        self.extension = spec.get_command_extension( cmd )
        self.vulkansc = spec.is_vulkansc_command( cmd )
