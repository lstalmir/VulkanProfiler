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

class LayerSettingAlias:
    def __init__( self, j: dict ):
        self.name = str(j["name"])
        self.env = str(j["env"]) if "env" in j.keys() else None
        self.settings = list(str(s) for s in j.get( "settings", [] ))

class LayerSettingFlag:
    def __init__( self, j: dict, index: int ):
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])
        self.platforms = list(j["platforms"] if "platforms" in j.keys() else [])
        self.index = index

class LayerSetting:
    def __init__( self, layer_info, j: dict, group: list[str] = [] ):
        self.group = list(group)
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])
        self.env = str(j["env"]) if "env" in j.keys() else None
        self.type = str(j["type"])
        self.default = j.get( "default" )
        self.platforms = list(j["platforms"] if "platforms" in j.keys() else [])
        if self.type == "ENUM":
            self.flags = [LayerSettingFlag(f, i) for i, f in enumerate( j["flags"] )]
        subsettings_group = list(self.group)
        if self.type == "GROUP":
            subsettings_group.append( self.key )
        self.settings = [LayerSetting( layer_info, s, subsettings_group ) for s in j.get( "settings", [] )]

    def c_type( self ):
        if self.type in ["ENUM", "GROUP"]:
            return f"{self.key}_t"
        if self.type in ["STRING", "LOAD_FILE", "SAVE_FILE"]:
            return "std::string"
        return self.type.lower()

    def c_name( self ):
        return f"m_{LayerSetting.c_make_identifier( self.key )}"

    def c_group( self ):
      return '.'.join( [f"m_{LayerSetting.c_make_identifier( key )}" for key in self.group] )

    def c_full_name( self ):
        name = self.c_name()
        group = self.c_group()
        if group:
            return f"{group}.{name}"
        return name

    def c_default( self ):
        if self.type == "GROUP":
            return "{}"
        if self.type == "BOOL":
            return "true" if self.default else "false"
        if self.type == "ENUM":
            return f"{self.c_type()}::{self.default}"
        if self.type in ["STRING", "LOAD_FILE", "SAVE_FILE"]:
            return f"\"{self.default}\""
        return str( self.default )

    def c_assign_from_string( self, string ):
        if self.type == "GROUP":
            raise Exception( "Cannot assign to GROUP setting from string." )
        if self.type in ["STRING", "LOAD_FILE", "SAVE_FILE"]:
            return f"{self.c_full_name()} = {string}"
        return f"TryParse({string}, {self.c_full_name()})"

    @staticmethod
    def c_make_identifier( name: str ):
        words = [v.capitalize() for v in name.split( '_' )]
        return ''.join( words )

class LayerInfo:
    def __init__( self, manifest: dict, aliases: dict ):
        j = manifest["layer"]
        self.name = j["name"]
        self.settings = self.__list_settings( j["features"] )
        self.aliases = [LayerSettingAlias(a) for a in aliases["aliases"]] if aliases else []

    def __list_settings( self, j: dict ):
        settings = []
        for setting in j.get( "settings", [] ):
            settings.append( LayerSetting( self, setting ) )
        return settings

    def __list_enums( self, settings ):
        for setting in settings:
            if setting.type == "ENUM":
                yield setting
            yield from self.__list_enums( setting.settings )

    def __list_groups( self, settings ):
        for setting in settings:
            yield from self.__list_groups( setting.settings )
            if setting.type == "GROUP":
                yield setting

    def __list_vars( self, settings ):
        for setting in settings:
            yield setting
            yield from self.__list_vars( setting.settings )

    def enumerate_enums( self ):
        yield from self.__list_enums( self.settings )

    def enumerate_groups( self ):
        yield from self.__list_groups( self.settings )

    def enumerate_vars( self ):
        yield from self.__list_vars( self.settings )

    def get_var( self, name ):
        for setting in self.enumerate_vars():
            if setting.key == name:
                return setting
        raise Exception( f"Setting '{name}' not found in layer '{self.name}'." )

def get_layer_info( path: str, aliases_path: str = None ):
    with open( path ) as file:
        manifest_json = json.load( file )

    aliases_json = None
    if aliases_path is not None:
        with open( aliases_path ) as file:
            aliases_json = json.load( file )

    return LayerInfo( manifest_json, aliases_json )

def merge_platforms( settings: list[LayerSetting] ):
    platforms = set()
    for setting in settings:
        platforms.update( setting.platforms )
    return list(platforms)

def begin_platforms( platforms: list[str] ):
    ifdef = ""
    if len( platforms ) > 0:
        ifdef = "#if 0"
        if "WINDOWS" in platforms:
            ifdef += " || defined( WIN32 )"
        if "LINUX" in platforms:
            ifdef += " || defined( __linux__ )"
        ifdef += "\n"
    return ifdef

def end_platforms( platforms: list[str] ):
    if len( platforms ) > 0:
        return "#endif\n"
    return ""

# C code generators.
def def_setting_LoadFromVulkanLayerSettings( out, layer_setting_set: str, key: str, settings: list[LayerSetting] ):
    out.write( f"    if(vkuHasLayerSetting({layer_setting_set}, \"{key}\")) {{\n" )
    for setting in settings:
        if setting.type == "ENUM":
            out.write( "      std::string value;\n" )
            out.write( f"      vkuGetLayerSettingValue({layer_setting_set}, \"{key}\", value);\n" )
            out.write( f"      {setting.c_assign_from_string( 'value' )};\n" )
        else:
            out.write( f"      vkuGetLayerSettingValue({layer_setting_set}, \"{key}\", {setting.c_full_name()});\n" )
    out.write( "    }\n" )

def def_settings_LoadFromVulkanLayerSettings( out, layer_setting_set: str, settings: list[LayerSetting] ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )
        if setting.type != "GROUP":
            def_setting_LoadFromVulkanLayerSettings( out, layer_setting_set, setting.key, [setting] )
        def_settings_LoadFromVulkanLayerSettings( out, layer_setting_set, setting.settings )
        out.write( end_platforms( setting.platforms ) )

def def_aliases_LoadFromVulkanLayerSettings( out, layer_setting_set: str, aliases: list[LayerSettingAlias], layer: LayerInfo ):
    for alias in aliases:
        settings = [layer.get_var( name ) for name in alias.settings]
        platforms = merge_platforms( settings )
        out.write( begin_platforms( platforms ) )
        def_setting_LoadFromVulkanLayerSettings( out, layer_setting_set, alias.name, settings )
        out.write( end_platforms( platforms ) )

def def_setting_LoadFromEnvironment( out, env: str, settings: list[LayerSetting] ):
    out.write( f"    if(auto var = ProfilerPlatformFunctions::GetEnvironmentVar(\"{env}\"); var.has_value()) {{\n" )
    for setting in settings:
        out.write( f"      {setting.c_assign_from_string( 'var.value()' )};\n" )
    out.write( "    }\n" )

def def_settings_LoadFromEnvironment( out, settings: list[LayerSetting] ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )
        if setting.env is not None:
            def_setting_LoadFromEnvironment( out, setting.env, [setting] )
        def_settings_LoadFromEnvironment( out, setting.settings )
        out.write( end_platforms( setting.platforms ) )

def def_aliases_LoadFromEnvironment( out, aliases: list[LayerSettingAlias], layer: LayerInfo ):
    for alias in aliases:
        if alias.env is not None:
            settings = [layer.get_var( name ) for name in alias.settings]
            platforms = merge_platforms( settings )
            out.write( begin_platforms( platforms ) )
            def_setting_LoadFromEnvironment( out, alias.env, settings )
            out.write( end_platforms( platforms ) )

def def_setting_LoadFromFile( out, name: str, value: str, key: str, settings: list[LayerSetting] ):
    out.write( f"        if(PROFILER_STREQI({name}.c_str(), \"{key}\")) {{\n" )
    for setting in settings:
        out.write( f"          {setting.c_assign_from_string( value )};\n" )
    out.write(  "          continue;\n" )
    out.write(  "        }\n" )

def def_settings_LoadFromFile( out, name: str, value: str, settings: list[LayerSetting] ):
    for setting in settings:
        out.write( begin_platforms( setting.platforms ) )
        if setting.type != "GROUP":
            def_setting_LoadFromFile( out, name, value, setting.key, [setting] )
        def_settings_LoadFromFile( out, name, value, setting.settings )
        out.write( end_platforms( setting.platforms ) )

def def_aliases_LoadFromFile( out, name: str, value: str, aliases: list[LayerSettingAlias], layer: LayerInfo ):
    for alias in aliases:
        settings = [layer.get_var( name ) for name in alias.settings]
        platforms = merge_platforms( settings )
        out.write( begin_platforms( platforms ) )
        def_setting_LoadFromFile( out, name, value, alias.name, settings )
        out.write( end_platforms( platforms ) )

# Generate configuration based on json file.
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser( description="Generate configuration based on json file." )
    parser.add_argument( "input", help="Path to the layer manifest json file.", type=str )
    parser.add_argument( "--aliases", help="Path to the layer setting aliases json file.", dest="aliases", type=str )
    parser.add_argument( "--output", "-o", help="Path to the generated cpp header file.", dest="output", type=str )
    args = parser.parse_args()

    layer = get_layer_info( args.input, args.aliases )
    with open( args.output, mode="w" ) as out:
        out.write( "#pragma once\n" )
        out.write( "#include <vulkan/layer/vk_layer_settings.hpp>\n" )
        out.write( "#include <charconv>\n" )
        out.write( "#include <string>\n" )
        out.write( "#include <fstream>\n" )
        out.write( "#include <filesystem>\n\n" )
        out.write( "#include \"profiler/profiler_helpers.h\"\n\n" )
        out.write( "namespace Profiler {\n\n" )

        # Declare enums
        for setting in layer.enumerate_enums():
            typename = setting.c_type()
            out.write( f"struct {typename} {{\n" )
            out.write( "  enum value_t {\n" )
            for flag in setting.flags:
                out.write( begin_platforms( flag.platforms ) )
                out.write( f"    // {flag.description}\n" )
                out.write( f"    {flag.key} = {flag.index},\n" )
                out.write( end_platforms( flag.platforms ) )
            out.write( "  } value;\n\n" )
            out.write( f"  {typename}(int v) : value(static_cast<value_t>(v)) {{}}\n" )
            out.write( f"  {typename}(const std::string& name) {{ TryParse(name, *this); }}\n" )
            out.write( "  operator value_t() const { return value; }\n\n" )
            out.write( f"  static bool TryParse(const std::string& value, {typename}& out) {{\n" )
            out.write( "    const char* pValue = value.c_str();\n" )
            for flag in setting.flags:
                out.write( begin_platforms( flag.platforms ) )
                out.write( f"    if(PROFILER_STREQI(\"{flag.index}\", pValue) || PROFILER_STREQI(\"{flag.key}\", pValue)) {{ out.value = {flag.key}; return true; }}\n" )
                out.write( end_platforms( flag.platforms ) )
            out.write( "    return false;\n" )
            out.write( "  }\n" )
            out.write( "};\n\n" )

        # Declare groups
        for setting in layer.enumerate_groups():
            out.write( f"struct {setting.c_type()} {{\n" )
            for s in setting.settings:
                out.write( f"  // {s.description}\n" )
                out.write( f"  {s.c_type()} {s.c_name()} = {s.c_default()};\n" )
            out.write( "};\n\n" )

        out.write( "struct ProfilerLayerSettings {\n" )
        for setting in layer.enumerate_vars():
            if not setting.group:
                out.write( f"  // {setting.description}\n" )
                out.write( f"  {setting.c_type()} {setting.c_name()} = {setting.c_default()};\n" )
        out.write( "\n" )

        out.write( "  // Value parsers\n" )
        out.write( "  template<typename T>\n" )
        out.write( "  static bool TryParse(const std::string& value, T& out) {\n" )
        out.write( "    return T::TryParse(value, out);\n" )
        out.write( "  }\n\n" )

        out.write( "  static bool TryParse(const std::string& value, bool& out) {\n" )
        out.write( "    const char* pValue = value.c_str();\n" )
        out.write( "    if(PROFILER_STREQI(pValue, \"1\") || PROFILER_STREQI(pValue, \"yes\") || PROFILER_STREQI(pValue, \"true\"))\n" )
        out.write( "      return (out = true), true;\n" )
        out.write( "    if(PROFILER_STREQI(pValue, \"0\") || PROFILER_STREQI(pValue, \"no\") || PROFILER_STREQI(pValue, \"false\"))\n" )
        out.write( "      return (out = false), true;\n" )
        out.write( "    return false;\n" )
        out.write( "  }\n\n" )

        out.write( "  static bool TryParse(const std::string& value, int& out) {\n" )
        out.write( "    const char* pValue = value.c_str();\n" )
        out.write( "    const char* pEnd = pValue + value.size();\n" )
        out.write( "    if(pValue[0] == '0' && (pValue[1] == 'x' || pValue[1] == 'X'))\n" )
        out.write( "      return std::from_chars(pValue + 2, pEnd, out, 16).ec == std::errc();\n" )
        out.write( "    return std::from_chars(pValue, pEnd, out, 10).ec == std::errc();\n" )
        out.write( "  }\n\n" )

        out.write( "  static bool TryParse(const std::string& value, float& out) {\n" )
        out.write( "    const char* pValue = value.c_str();\n" )
        out.write( "    const char* pEnd = pValue + value.size();\n" )
        out.write( "    return std::from_chars(pValue, pEnd, out).ec == std::errc();\n" )
        out.write( "  }\n\n" )

        out.write( "  // Support Vulkan layer settings\n" )
        out.write( "  void LoadFromVulkanLayerSettings(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator) {\n" )
        out.write( "    const VkLayerSettingsCreateInfoEXT* pLayerSettingsCreateInfo = vkuFindLayerSettingsCreateInfo(pCreateInfo);\n\n" )
        out.write( "    VkuLayerSettingSet layerSettingSet = VK_NULL_HANDLE;\n" )
        out.write( f"    vkuCreateLayerSettingSet(\"{layer.name}\", pLayerSettingsCreateInfo, pAllocator, nullptr, &layerSettingSet);\n\n" )
        out.write( "    // Load aliases\n" )
        def_aliases_LoadFromVulkanLayerSettings( out, "layerSettingSet", layer.aliases, layer )
        out.write( "\n" )
        out.write( "    // Load settings\n" )
        def_settings_LoadFromVulkanLayerSettings( out, "layerSettingSet", layer.settings )
        out.write( "    vkuDestroyLayerSettingSet(layerSettingSet, pAllocator);\n" )
        out.write( "  }\n\n" )

        out.write( "  // Support loading from environment\n" )
        out.write( "  void LoadFromEnvironment() {\n" )
        out.write( "    // Load aliases\n" )
        def_aliases_LoadFromEnvironment( out, layer.aliases, layer )
        out.write( "    // Load settings\n" )
        def_settings_LoadFromEnvironment( out, layer.settings )
        out.write( "  }\n\n" )
        
        out.write( "  // Support legacy config file\n" )
        out.write( "  void LoadFromFile(const std::filesystem::path& path) {\n" )
        out.write( "    std::ifstream in(path);\n" )
        out.write( "    std::string name, value;\n" )
        out.write( "    while(in) {\n" )
        out.write( "      in >> name >> value;\n" )
        out.write( "      if(!name.empty()) {\n" )
        out.write( "        // Load aliases\n" )
        def_aliases_LoadFromFile( out, "name", "value", layer.aliases, layer )
        out.write( "        // Load settings\n" )
        def_settings_LoadFromFile( out, "name", "value", layer.settings )
        out.write( "      }\n" )
        out.write( "    }\n" )
        out.write( "  }\n" )

        out.write( "};\n\n" )
        out.write( "}\n" )
