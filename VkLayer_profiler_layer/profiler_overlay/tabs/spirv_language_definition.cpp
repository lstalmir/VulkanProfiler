// Copyright (c) 2019-2022 Lukasz Stalmirski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "../external/ImGuiColorTextEdit/TextEditor.h"

#include "spirv/unified1/spirv.h"
#include "spirv-tools/libspirv.h"

namespace Profiler
{
    using LanguageDefinition = TextEditor::LanguageDefinition;
    using PaletteIndex = TextEditor::PaletteIndex;

    const LanguageDefinition& GetSpirvLanguageDefinition()
    {
        static bool inited = false;
        static LanguageDefinition langDef;
        if( !inited )
        {
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "Op[a-zA-Z0-9]+", PaletteIndex::Keyword ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "%[a-zA-Z0-9_]+", PaletteIndex::Identifier ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number ) );
            langDef.mTokenRegexStrings.push_back( std::make_pair<std::string, PaletteIndex>( "[\\[\\]\\{\\}\\!\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\,\\.]", PaletteIndex::Punctuation ) );

            langDef.mCommentStart = ';';
            langDef.mCommentEnd = '\n';
            langDef.mSingleLineComment = ';';

            langDef.mCaseSensitive = true;
            langDef.mAutoIndentation = true;

            langDef.mName = "SPIR-V";

            inited = true;
        }
        return langDef;
    }
}
