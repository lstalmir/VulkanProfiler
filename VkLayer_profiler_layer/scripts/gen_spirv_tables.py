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

class SpirvDocumentationParser:
    def __init__( self, spirv_doc_path: str ):
        self.bs = None
        try:
            import BeautifulSoup as bs
            self.bs = bs
        except ImportError:
            try:
                import bs4 as bs
                self.bs = bs
            except ImportError:
                print("BeautifulSoup module not found. SPIR-V documentation will not be available in this build.")

        self.doc = None
        if self.bs is not None:
            with open( spirv_doc_path ) as f:
                self.doc = self.bs.BeautifulSoup( f, "html.parser" )

    @staticmethod
    def normalize( s: str, first: int = 0 ):
        lines = s.splitlines()[first:]
        lines = [line for line in lines if len(line) > 0]
        s = "\\n".join( lines )
        s = s.replace("\"", "\\\"")
        return s

    def get_op_doc( self, op: str ) -> str:
        try:
            return SpirvDocumentationParser.normalize( self.doc.find("a", id=op).parent.text, 1 )
        except:
            return ""

    def get_op_operands( self, op: str ) -> list:
        try:
            return [SpirvDocumentationParser.normalize( operand.text )
                    for operand in self.doc.find("a", id=op).findParent("tr").findNextSibling("tr")
                    if operand != "\n"]
        except:
            return []

def get_spirv_ops( spirv_json_path: str ) -> list:
    with open( spirv_json_path ) as f:
        spirv_json = json.load( f )
    for enum in spirv_json["spv"]["enum"]:
        if enum["Name"] == "Op":
            return enum["Values"].keys()
    raise KeyError()

def gen_spirv_tables( spirv_json_path: str, spirv_grammar_path: str, spirv_doc_path: str, out_header_path: str ):
    with open( spirv_json_path ) as f:
        spirv_json = json.load( f )

    spirv_docs = SpirvDocumentationParser( spirv_doc_path )

    with open( out_header_path, "w" ) as h:
        h.write( "#pragma once\n" )
        h.write( "#include <spirv/unified1/spirv.h>\n\n" )

        h.write( "namespace Profiler\n{\n" )
        h.write( "struct SpirvOpCodeDesc {\n" )
        h.write( "  SpvOp       m_OpCode;\n" )
        h.write( "  const char* m_pName;\n" )
        h.write( "  const char* m_pDoc;\n" )
        h.write( "  uint32_t    m_OperandsCount;\n" )
        h.write( "  const char* m_pOperands[32];\n" )
        h.write( "};\n\n" )

        h.write( "static constexpr SpirvOpCodeDesc SpirvOps[] = {\n" )
        for enum in spirv_json["spv"]["enum"]:
            if enum["Name"] == "Op":
                for name in enum["Values"].keys():
                    doc = spirv_docs.get_op_doc( name )
                    operands = spirv_docs.get_op_operands( name )
                    operands_count = len( operands )
                    h.write( f"  {{ Spv{name}, \"{name}\", \"{doc}\", {operands_count}, {{ " )
                    for operand in operands:
                        h.write( f"\"{operand}\", " )
                    h.write( "} },\n" )
                break
        h.write( "};\n" )

        h.write( "} // namespace Profiler\n" )

if __name__ == "__main__":
    gen_spirv_tables(
        spirv_json_path=os.path.abspath( sys.argv[1] ),
        spirv_grammar_path=os.path.abspath( sys.argv[2] ),
        spirv_doc_path=os.path.abspath( sys.argv[3] ),
        out_header_path=os.path.abspath( sys.argv[4] ) )