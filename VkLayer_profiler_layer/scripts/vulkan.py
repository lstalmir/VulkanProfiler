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

import xml.etree.ElementTree as et

class Enum:
    def __init__( self, name: str, value: int, protect: str = None ):
        self.name = name
        self.value = value
        self.protect = protect

class Registry:
    def __init__( self, vk_xml: et.ElementTree ):
        self.xml = vk_xml.getroot()
        self.types = self.xml.find( 'types' )
        self.commands = self.xml.find( 'commands' )
        self.extensions = self.xml.find( 'extensions' )
        self.vulkansc = self.xml.find( 'feature[@api="vulkansc"]' )
        self.handles = [handle.text for handle in self.types.findall( 'type[@category="handle"][type="VK_DEFINE_HANDLE"]/name' )]
        self.enable_beta_extensions = False

    def get_command_name( self, cmd: et.Element ):
        try: # Function definition
            return cmd.find( 'proto/name' ).text
        except: # Function alias
            return cmd.get( 'name' )

    def get_command_handle( self, cmd: et.Element ):
        try: # Function definition
            return cmd.find( 'param[1]/type' ).text
        except: # Function alias
            cmd_alias = cmd.get( "alias" )
            return self.commands.find( f'command/proto[name="{cmd_alias}"]/../param[1]/type' ).text

    def get_command_extension( self, cmd: et.Element ):
        for supported in ("vulkan", "vulkan,vulkansc"):
            try: # Extension function
                cmd_name = self.get_command_name( cmd )
                return self.extensions.find( f'extension[@supported="{supported}"]/require/command[@name="{cmd_name}"]/../..' ).get( 'name' )
            except AttributeError: # Core function
                continue
        return None

    def is_vulkansc_command( self, cmd: et.Element ):
        # Check if cmd is secure version of a standard functions
        if cmd.attrib.get( 'api' ) == "vulkansc":
            return True
        # Check if cmd is required by vulkansc feature set
        cmd_name = self.get_command_name( cmd )
        if self.vulkansc.find( f'require/command[@name="{cmd_name}"]' ) is not None:
            return True
        # Check if cmd is required by any vulkansc extension
        if self.extensions.find( f'extension[@supported="vulkansc"]/require/command[@name="{cmd_name}"]' ) is not None:
            return True
        return False

    def is_disabled_command( self, cmd: et.Element ):
        # Check if cmd is required by a disabled extension
        cmd_name = self.get_command_name( cmd )
        if self.extensions.find( f'extension[@supported="disabled"]/require/command[@name="{cmd_name}"]' ) is not None:
            return True
        return False

    def get_enums( self, enum_name: str ):
        # Check if enum_name is a flags type
        flags_type = self.types.find( f'type[@category="bitmask"]/name[.="{enum_name}"]/..' )
        if flags_type is not None:
            if 'alias' in flags_type.attrib:
                return self.get_enums( flags_type.get( 'alias' ) )
            enum_name = flags_type.get( 'requires' )
        if enum_name is None:
            return None

        # Check if enum_name is an enum type alias
        enum_type = self.types.find( f'type[@category="enum"][@name="{enum_name}"]' )
        if enum_type is not None:
            if 'alias' in enum_type.attrib:
                return self.get_enums( enum_type.get( 'alias' ) )

        # Return core enum values
        for enum in self.xml.findall( f'enums[@name="{enum_name}"]/enum' ):
            resolved_enum = self._resolve_enum( enum )
            if resolved_enum is not None:
                yield resolved_enum

        # Return promoted enum values
        for enum in self.xml.findall( f'feature/require/enum[@extends="{enum_name}"]' ):
            resolved_enum = self._resolve_enum( enum )
            if resolved_enum is not None:
                yield resolved_enum

        # Return extension enum values
        for supported in ("vulkan", "vulkan,vulkansc"):
            for enum in self.extensions.findall( f'extension[@supported="{supported}"]/require/enum[@extends="{enum_name}"]' ):
                resolved_enum = self._resolve_enum( enum )
                if resolved_enum is not None:
                    yield resolved_enum

    def _resolve_enum( self, enum: et.Element ):
        name = enum.get( 'name' )
        if name is None:
            return None

        # Check if enum is an alias
        if 'alias' in enum.attrib:
            return None

        # Check if enum is part of a beta extension
        protect = enum.get( 'protect' )
        if protect == 'VK_ENABLE_BETA_EXTENSIONS' and not self.enable_beta_extensions:
            return None

        # Return simple enum value
        value = enum.get( 'value' )
        if value is not None:
            if value.startswith( '0x' ):
                return Enum( name, int( value[2:], 16 ), protect )
            return Enum( name, int( value ), protect )

        # Return bitmask enum value
        bitpos = enum.get( 'bitpos' )
        if bitpos is not None:
            return Enum( name, 1 << int( bitpos ), protect )

        # Return extension enum value
        offset = enum.get( 'offset' )
        if offset is not None:
            extnumber = enum.get( 'extnumber' )
            if extnumber is None:
                for supported in ("vulkan", "vulkan,vulkansc"):
                    extension = self.extensions.find( f'extension[@supported="{supported}"]/require/enum[@name="{name}"]/../..' )
                    if extension is not None:
                        extnumber = extension.get( 'number' )
                        break
            if extnumber is not None:
                base_value = 1000000000
                range_size = 1000
                value = base_value + ( int( extnumber ) - 1 ) * range_size + int( offset )
                return Enum( name, value, protect )

        return None

def load_registry( vk_xml_path: str ):
    return Registry( et.parse( vk_xml_path ) )
