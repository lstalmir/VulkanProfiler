// Copyright (c) 2020 Lukasz Stalmirski
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

#include "Args.h"
#include <cstring>

namespace Sample
{
    Args::Args( int argc, const char** argv )
    {
        Argv = std::vector<const char*>( argc );
        std::memcpy( Argv.data(), argv, argc * sizeof( *argv ) );
    }

    Args::~Args()
    {
        Argv.clear();
    }

    bool Args::IsSet( const char* opt )
    {
        const size_t argc = Argv.size();
        for( size_t i = 0; i < argc; ++i )
        {
            if( (std::strcmp( Argv[ i ], opt ) == 0) )
                return true;
        }
        return false;
    }

    const char* Args::Get( const char* opt )
    {
        const size_t argc = Argv.size();
        for( size_t i = 0; i < argc; ++i )
        {
            if( (std::strcmp( Argv[ i ], opt ) == 0) && (i < argc - 1) )
                return Argv[ i + 1 ];
        }
        return nullptr;
    }
}
