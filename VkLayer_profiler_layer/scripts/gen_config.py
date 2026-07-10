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

import json
from types import NoneType

def find_setting( settings: list, key: str, group: str | NoneType = None ):
    if group is not None:
        group_path = group.split('.')

def make_c_name( name: str ):
    c_name_components = []
    for component in name.split('.'):
        words = [v.capitalize() for v in component.split('_')]
        c_name_components.append( f"m_{''.join(words)}" )
    return ".".join( c_name_components )

class LayerSettingFlag:
    def __init__( self, j: dict ):
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])

    def __eq__( self, other ):
        if not isinstance( other, LayerSettingFlag ):
            return False
        return self.key == other.key

class LayerSettingDependence:
    def __init__( self, j: dict ):
        self.mode = j.get( "mode", "NONE" )
        self.settings = {s["key"]: s["value"] for s in j.get( "settings", [] )}

# Base class for all layer settings
class LayerSetting:
    def __init__( self, layer_info, j: dict, group: str | NoneType = None ):
        self.group = group
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])
        self.type = str(j["type"])
        self.env = j.get( "env", None )
        self.default = j.get( "default", None )
        self.platforms = j.get( "platforms", [] )
        self.dependence = LayerSettingDependence( j.get( "dependence", {} ) )

    def __eq__( self, other ):
        if not isinstance( other, LayerSetting ):
            return False
        return self.key == other.key and \
            self.type == other.type and \
            self.default == other.default and \
            self.env == other.env

    def c_type( self ):
        return self.type.lower()

    def c_name( self ):
        return make_c_name( self.key )

    def c_full_name( self ):
        if self.group is not None:
            return f"{make_c_name( self.group )}.{make_c_name( self.key )}"
        return make_c_name( self.key )

    def c_value( self, value ):
        return str( value )

    def c_default( self ):
        if self.default is None:
            return "{}"
        return self.c_value( self.default )

    def c_assign_from_string( self, string ):
        return f"{self.c_full_name()} = static_cast<{self.c_type()}>(std::stoi({string}))"

    def c_dependence_condition( self, settings ):
        if self.dependence.mode != "NONE":
            conditions = []
            for key, value in self.dependence.settings.items():
                setting = find_setting( settings, key, self.group )
                if setting is not None:
                    conditions.append( f"({setting.c_full_name()} == {setting.c_value( value )})" )
            if self.dependence.mode == "ALL":
                return " && ".join( conditions )
            if self.dependence.mode == "ANY":
                return " || ".join( conditions )
        return None

# BOOL setting
class BoolLayerSetting(LayerSetting):
    def __init__( self, layer_info, j: dict, group: str | NoneType = None ):
        super().__init__( layer_info, j, group )
        
    def __eq__( self, other ):
        if not isinstance( other, BoolLayerSetting ):
            return False
        return super().__eq__( other )

    def c_value( self, value ):
        return "true" if value else "false"

    def c_assign_from_string( self, string ):
        return f"bool_t::TryParse({string}.c_str(), {self.c_full_name()})"

# ENUM setting
class EnumLayerSetting(LayerSetting):
    def __init__( self, layer_info, j: dict, group: str | NoneType = None ):
        super().__init__( layer_info, j, group )
        self.flags = [LayerSettingFlag(f) for f in j.get( "flags", [] )]
        
    def __eq__( self, other ):
        if not isinstance( other, EnumLayerSetting ):
            return False
        return super().__eq__( other ) and \
            self.flags == other.flags

    def c_type( self ):
        return f"{self.key}_t"

    def c_value( self, value ):
        return f"{self.c_type()}::{value}"

    def c_assign_from_string( self, string ):
        return f"{self.c_type()}::TryParse({string}.c_str(), {self.c_full_name()})"

# STRING-like setting
class StringLayerSetting(LayerSetting):
    def __init__( self, layer_info, j: dict, group: str | NoneType = None ):
        super().__init__( layer_info, j, group )
        
    def __eq__( self, other ):
        if not isinstance( other, StringLayerSetting ):
            return False
        return super().__eq__( other )

    def c_type( self ):
        return "std::string"

    def c_value( self, value ):
        return f"\"{value}\""

    def c_assign_from_string( self, string ):
        return f"{self.c_full_name()} = {string}"

# GROUP setting
class GroupLayerSetting(LayerSetting):
    def __init__( self, layer_info, j: dict, group: str | NoneType = None ):
        super().__init__( layer_info, j, group )
        subgroup = f"{group}.{self.key}" if group is not None else self.key
        self.settings = layer_info.parse_layer_settings_list( j["settings"], subgroup )

    def c_type( self ):
        return f"{self.key}_t"

class LayerInfo:
    def __init__( self, j: dict ):
        self.name = j["name"]
        self.settings = self.parse_layer_settings_list( j["features"]["settings"] )
        
    def parse_layer_setting( self, j: dict, group: str | NoneType = None ):
        setting_type = j["type"]
        if setting_type == "GROUP":
            return GroupLayerSetting( self, j, group )
        elif setting_type == "BOOL":
            return BoolLayerSetting( self, j, group )
        elif setting_type == "ENUM":
            return EnumLayerSetting( self, j, group )
        elif setting_type in ["STRING", "LOAD_FILE", "SAVE_FILE"]:
            return StringLayerSetting( self, j, group )
        else:
            return LayerSetting( self, j, group )

    def parse_layer_settings_list( self, j: dict, group: str | NoneType = None ):
        settings = []
        for setting in j:
            layer_setting = self.parse_layer_setting( setting, group )
            if layer_setting.type == "GROUP":
                settings.append( layer_setting )
            else:
                settings.append( layer_setting )
                subsettings = setting.get( "settings", None )
                if subsettings is not None:
                    settings.extend( self.parse_layer_settings_list( subsettings, group ) )
        return settings

def get_layer_info( path: str ):
    with open( path ) as file:
        manifest_json = json.load( file )
    return LayerInfo( manifest_json["layer"] )

def get_unique_enums( settings: list[LayerSetting] ):
    enums = {}
    def append_unique_enum( setting: LayerSetting ):
        try:
            existing_definition = enums[setting.key]
            if existing_definition != setting:
                raise ValueError( f"Duplicate enum definition for key '{setting.key}'" )
        except KeyError:
            enums[setting.key] = setting
    for setting in settings:
        if setting.type == "ENUM":
            append_unique_enum( setting )
        elif setting.type == "GROUP":
            for group_setting in get_unique_enums( setting.settings ):
                append_unique_enum( group_setting )
    return enums.values()

def begin_platforms( platforms: list[str] ):
    ifdef = ""
    if len( platforms ) > 0:
        ifdef = "#if 1"
        if "WINDOWS" in platforms:
            ifdef += " && defined( WIN32 )"
        if "LINUX" in platforms:
            ifdef += " && defined( __linux__ )"
        ifdef += "\n"
    return ifdef

def end_platforms( platforms: list[str] ):
    if len( platforms ) > 0:
        return "#endif\n"
    return ""

def def_LoadFromVulkanLayerSettings( out, settings ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )

        if setting.type == "GROUP":
            def_LoadFromVulkanLayerSettings( out, setting.settings )

        else:
            out.write( f"    if(vkuHasLayerSetting(layerSettingSet, \"{setting.key}\")) {{\n" )
            if setting.type == "ENUM":
                out.write( "      std::string value;\n" )
                out.write( f"      vkuGetLayerSettingValue(layerSettingSet, \"{setting.key}\", value);\n" )
                out.write( f"      {setting.c_assign_from_string('value')};\n" )
            else:
                out.write( f"      vkuGetLayerSettingValue(layerSettingSet, \"{setting.key}\", {setting.c_full_name()});\n" )
            out.write( "    }\n" )

        out.write( end_platforms( setting.platforms ) )

def def_LoadFromEnvironment( out, settings ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )

        if setting.type == "GROUP":
            def_LoadFromEnvironment( out, setting.settings )

        elif setting.env is not None:
            out.write( f"    if(auto var = ProfilerPlatformFunctions::GetEnvironmentVar(\"{setting.env}\"); var.has_value()) {{\n" )
            out.write( f"      {setting.c_assign_from_string('var.value()')};\n" )
            out.write( "    }\n" )

        out.write( end_platforms( setting.platforms ) )

def def_LoadFromFile( out, settings ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )

        if setting.type == "GROUP":
            def_LoadFromFile( out, setting.settings )

        else:
            out.write( f"        if(PROFILER_STREQI(name.c_str(), \"{setting.key}\")) {{\n" )
            out.write( f"          {setting.c_assign_from_string('value')};\n" )
            out.write(  "          continue;\n" )
            out.write(  "        }\n" )

        out.write( end_platforms( setting.platforms ) )

# Generate configuration based on json file.
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser( description="Generate configuration based on json file." )
    parser.add_argument( "input", help="Path to the layer manifest json file.", type=str )
    parser.add_argument( "--output", "-o", help="Path to the generated cpp header file.", dest="output", type=str )
    args = parser.parse_args()

    layer = get_layer_info( args.input )
    with open( args.output, mode="w" ) as out:
        out.write( "#pragma once\n" )
        out.write( "#include <vulkan/layer/vk_layer_settings.hpp>\n" )
        out.write( "#include <string>\n" )
        out.write( "#include <fstream>\n" )
        out.write( "#include <filesystem>\n\n" )
        out.write( "#include \"profiler/profiler_helpers.h\"\n\n" )
        out.write( "namespace Profiler {\n\n" )

        out.write( "struct bool_t {\n" )
        out.write( "  static bool TryParse(const char* pName, bool& out) {\n" )
        out.write( "    if(PROFILER_STREQI(pName, \"1\") || PROFILER_STREQI(pName, \"yes\") || PROFILER_STREQI(pName, \"true\"))\n" )
        out.write( "      return (out = true), true;\n" )
        out.write( "    if(PROFILER_STREQI(pName, \"0\") || PROFILER_STREQI(pName, \"no\") || PROFILER_STREQI(pName, \"false\"))\n" )
        out.write( "      return (out = false), true;\n" )
        out.write( "    return false;\n" )
        out.write( "  }\n" )
        out.write( "};\n\n" )

        # Declare enums
        for setting in get_unique_enums( layer.settings ):
            typename = setting.c_type()
            out.write( f"struct {typename} {{\n" )
            out.write( "  enum value_t {\n" )
            for flag in setting.flags:
                out.write( f"    // {flag.description}\n" )
                out.write( f"    {flag.key},\n" )
            out.write( "  } value;\n\n" )
            out.write( f"  {typename}(int v) : value(static_cast<value_t>(v)) {{}}\n" )
            out.write( f"  {typename}(const std::string& name) : {typename}(name.c_str()) {{}}\n" )
            out.write( f"  {typename}(const char* pName) : value({setting.c_default()}) {{ TryParse(pName, *this); }}\n" )
            out.write( "  operator value_t() const { return value; }\n\n" )
            out.write( f"  static bool TryParse(const char* pName, {typename}& out) {{\n" )
            for idx, flag in enumerate(setting.flags):
                out.write( f"    if(PROFILER_STREQI(\"{idx}\", pName) || PROFILER_STREQI(\"{flag.key}\", pName)) {{ out.value = {flag.key}; return true; }}\n" )
            out.write( "    return false;\n" )
            out.write( "  }\n" )
            out.write( "};\n\n" )

        # Declare groups
        for setting in layer.settings:
            if setting.type == "GROUP":
                out.write( begin_platforms( setting.platforms ) )
                out.write( f"struct {setting.c_type()} {{\n" )
                for subsetting in setting.settings:
                    out.write( f"  // {subsetting.description}\n" )
                    out.write( f"  {subsetting.c_type()} {subsetting.c_name()} = {subsetting.c_default()};\n" )
                out.write( "\n" )
                out.write( "};\n\n" )
                out.write( end_platforms( setting.platforms ) )

        out.write( "struct ProfilerLayerSettings {\n" )
        for setting in layer.settings:
            out.write( f"  // {setting.description}\n" )
            out.write( f"  {setting.c_type()} {setting.c_name()} = {setting.c_default()};\n" )
        out.write( "\n" )

        out.write( "  // Support Vulkan layer settings\n" )
        out.write( "  void LoadFromVulkanLayerSettings(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator) {\n" )
        out.write( "    const VkLayerSettingsCreateInfoEXT* pLayerSettingsCreateInfo = vkuFindLayerSettingsCreateInfo(pCreateInfo);\n\n" )
        out.write( "    VkuLayerSettingSet layerSettingSet = VK_NULL_HANDLE;\n" )
        out.write( f"    vkuCreateLayerSettingSet(\"{layer.name}\", pLayerSettingsCreateInfo, pAllocator, nullptr, &layerSettingSet);\n\n" )
        def_LoadFromVulkanLayerSettings( out, layer.settings )
        out.write( "    vkuDestroyLayerSettingSet(layerSettingSet, pAllocator);\n" )
        out.write( "  }\n\n" )

        out.write( "  // Support loading from environment\n" )
        out.write( "  void LoadFromEnvironment() {\n" )
        def_LoadFromEnvironment( out, layer.settings )
        out.write( "  }\n\n" )

        out.write( "  // Support legacy config file\n" )
        out.write( "  void LoadFromFile(const std::filesystem::path& path) {\n" )
        out.write( "    std::ifstream in(path);\n" )
        out.write( "    std::string name, value;\n" )
        out.write( "    while(in) {\n" )
        out.write( "      in >> name >> value;\n" )
        out.write( "      if(!name.empty()) {\n" )
        def_LoadFromFile( out, layer.settings )
        out.write( "      }\n" )
        out.write( "    }\n" )
        out.write( "  }\n" )

        out.write( "};\n\n" )
        out.write( "}\n" )
