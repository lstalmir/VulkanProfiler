import sys
import os.path

from docutils.parsers.rst.directives.misc import Include

vkprof_docs_dir = os.path.dirname( os.path.realpath( __file__ ) )
vkprof_root_dir = os.path.dirname( vkprof_docs_dir )
vkprof_licenses = f'{vkprof_root_dir}/LICENSE.md'

# -- Directives --------------------------------------------------------------
class VkProfIncludeLicenses( Include ):
    required_arguments = 0

    def run( self ):
        config = self.state.document.settings.env.config
        licenses = config[ 'vkprof_licenses' ]
        if not licenses:
            licenses = vkprof_licenses
        if not os.path.isabs( licenses ):
            licenses = os.path.join( vkprof_root_dir, licenses )
        print( f'including licenses from {licenses}' )
        self.arguments = [ licenses ]
        return super().run()

# -- Config options ----------------------------------------------------------
def setup( app ):
    app.add_config_value( 'vkprof_licenses', vkprof_licenses, 'html' )
    app.add_directive( 'vkprof_include_licenses', VkProfIncludeLicenses )
