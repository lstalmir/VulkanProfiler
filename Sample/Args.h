#pragma once
#include <vector>
#include <string>
#include <optional>

namespace Sample
{
    // Singleton
    struct Args
    {
        inline static std::vector<const char*> Argv;

        Args( int argc, const char** argv );
        ~Args();

        static bool IsSet( const char* );
        static const char* Get( const char* );
    };
}
