#include "Args.h"

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
