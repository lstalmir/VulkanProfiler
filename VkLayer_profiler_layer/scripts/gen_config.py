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

from email.policy import default
import io
import json

class LayerSettingFlag:
    def __init__( self, j: dict ):
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])

class LayerSetting:
    def __init__( self, layer_info, j: dict ):
        self.key = str(j["key"])
        self.label = str(j["label"])
        self.description = str(j["description"])
        self.env = str(j["env"]) if "env" in j.keys() else None
        self.type = str(j["type"])
        self.default = j["default"]
        self.platforms = list(j["platforms"] if "platforms" in j.keys() else [])
        if self.type == "ENUM":
            self.flags = [LayerSettingFlag(f) for f in j["flags"]]

    def c_type( self ):
        if self.type == "ENUM":
            return f"{self.key}_t"
        return self.type.lower()

    def c_default( self ):
        if self.type == "BOOL":
            return "true" if self.default else "false"
        if self.type == "ENUM":
            return f"{self.c_type()}::{self.default}"
        return str( self.default )

    def c_assign_from_string( self, string ):
        if self.type == "BOOL":
            return f"bool_t::TryParse({string}.c_str(), {self.key})"
        if self.type == "ENUM":
            return f"{self.c_type()}::TryParse({string}.c_str(), {self.key})"
        if self.type == "STRING":
            return f"{self.key} = {string}"
        return f"{self.key} = static_cast<{self.c_type()}>(std::stoi({string}))"

class LayerInfo:
    def __init__( self, j: dict ):
        self.name = j["name"]
        self.settings = [LayerSetting( self, setting ) for setting in j["features"]["settings"]]

def get_layer_info( path: str ):
    with open( path ) as file:
        manifest_json = json.load( file )
    return LayerInfo( manifest_json["layer"] )

def begin_platforms( platforms: list[str] ):
    ifdef = ""
    if len(platforms) > 0:
        ifdef = "#if 1"
        if "WINDOWS" in platforms:
            ifdef += " && defined( WIN32 )"
        if "LINUX" in platforms:
            ifdef += " && defined( __linux__ )"
        ifdef += "\n"
    return ifdef

def end_platforms( platforms: list[str] ):
    if len(platforms) > 0:
        return "#endif\n"
    return ""

# Generate configuration bsed on json file.
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
        out.write( "    if(!strcmp(pName, \"1\") || !strcmp(pName, \"yes\") || !strcmp(pName, \"true\"))\n" )
        out.write( "      return (out = true), true;\n" )
        out.write( "    if(!strcmp(pName, \"0\") || !strcmp(pName, \"no\") || !strcmp(pName, \"false\"))\n" )
        out.write( "      return (out = false), true;\n" )
        out.write( "    return false;\n" )
        out.write( "  }\n" )
        out.write( "};\n\n" )

        # Declare enums
        for setting in layer.settings:
            if setting.type == "ENUM":
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
                    out.write( f"    if(!strcmp(\"{idx}\", pName) || !strcmp(\"{flag.key}\", pName)) {{ out.value = {flag.key}; return true; }}\n" )
                out.write( "    return false;\n" )
                out.write( "  }\n" )
                out.write( "};\n\n" )

        out.write( "struct ProfilerLayerSettings {\n" )
        for setting in layer.settings:
            out.write( f"  // {setting.description}\n" )
            out.write( f"  {setting.c_type()} {setting.key} = {setting.c_default()};\n" )
        out.write( "\n" )

        out.write( "  // Support Vulkan layer settings\n" )
        out.write( "  void LoadFromVulkanLayerSettings(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator) {\n" )
        out.write( "    const VkLayerSettingsCreateInfoEXT* pLayerSettingsCreateInfo = vkuFindLayerSettingsCreateInfo(pCreateInfo);\n\n" )
        out.write( "    VkuLayerSettingSet layerSettingSet = VK_NULL_HANDLE;\n" )
        out.write( f"    vkuCreateLayerSettingSet(\"{layer.name}\", pLayerSettingsCreateInfo, pAllocator, nullptr, &layerSettingSet);\n\n" )
        for setting in layer.settings:
            out.write( begin_platforms( setting.platforms ) )
            out.write( f"    if(vkuHasLayerSetting(layerSettingSet, \"{setting.key}\")) {{\n" )
            if setting.type == "ENUM":
                out.write( "      std::string value;\n" )
                out.write( f"      vkuGetLayerSettingValue(layerSettingSet, \"{setting.key}\", value);\n" )
                out.write( f"      {setting.key} = {setting.c_type()}(value);\n" )
            else:
                out.write( f"      vkuGetLayerSettingValue(layerSettingSet, \"{setting.key}\", {setting.key});\n" )
            out.write( "    }\n" )
            out.write( end_platforms( setting.platforms ) )
        out.write( "    vkuDestroyLayerSettingSet(layerSettingSet, pAllocator);\n" )
        out.write( "  }\n\n" )

        out.write( "  // Support loading from environment\n" )
        out.write( "  void LoadFromEnvironment() {\n" )
        for setting in layer.settings:
            if setting.env is not None:
                out.write( begin_platforms( setting.platforms ) )
                out.write( f"    if(auto var = ProfilerPlatformFunctions::GetEnvironmentVar(\"{setting.env}\"); var.has_value()) {{\n" )
                out.write( f"      {setting.c_assign_from_string('var.value()')};\n" )
                out.write( "    }\n" )
                out.write( end_platforms( setting.platforms ) )
        out.write( "  }\n\n" )
        
        out.write( "  // Support legacy config file\n" )
        out.write( "  void LoadFromFile(const std::filesystem::path& path) {\n" )
        out.write( "    std::ifstream in(path);\n" )
        out.write( "    std::string name, value;\n" )
        out.write( "    while(in) {\n" )
        out.write( "      in >> name >> value;\n" )
        out.write( "      if(!name.empty()) {\n" )
        for setting in layer.settings:
            out.write( f"        if(!strcmp(name.c_str(), \"{setting.key}\")) {{\n" )
            out.write( f"          {setting.c_assign_from_string('value')};\n" )
            out.write(  "          continue;\n" )
            out.write(  "        }\n" )
        out.write( "      }\n" )
        out.write( "    }\n" )
        out.write( "  }\n" )

        out.write( "};\n\n" )
        out.write( "}\n" )
