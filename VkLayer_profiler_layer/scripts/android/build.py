# Copyright (c) 2025 Lukasz Stalmirski
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

import argparse
import subprocess
import os

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument( "source", help="Source directory", default=os.getcwd() )

    # Android build options
    parser.add_argument( "--ndk", help="Path to the Android NDK", required=True )
    parser.add_argument( "--abi", help="Target ABI", default="arm64-v8a" )

    # CMake configuration options
    parser.add_argument( "-B", help="Build directory", dest="build_dir", default=os.getcwd() )
    parser.add_argument( "-G", help="CMake generator", dest="generator" )
    parser.add_argument( "--config", help="Build configuration", default="Release" )
    parser.add_argument( "--parallel", "-j", help="Number of concurrent build jobs", default="4" )
    parser.add_argument( "--clean", help="Remove CMakeCache.txt to force reconfiguration of the makefiles", default=False, action="store_true" )

    # Actions
    parser.add_argument( "--install", help="Install the layer to the specified location", dest="install_dir" )

    return parser.parse_args()

def run_command( command ):
    print( f"Running command: {' '.join( command )}" )
    subprocess.run( command )

def configure( args ):
    cmake_toolchain_file = os.path.join( args.ndk, "build", "cmake", "android.toolchain.cmake" )
    if not os.path.exists( cmake_toolchain_file ):
        raise FileNotFoundError( cmake_toolchain_file )

    if args.clean:
        cmake_cache = os.path.join( args.build_dir, "CMakeCache.txt" )
        if os.path.exists( cmake_cache ):
            print( f"Removing {cmake_cache}" )
            os.remove( cmake_cache )

    cmake_command = [ "cmake", args.source ]
    if args.generator:
        cmake_command += [ "-G", args.generator ]
    cmake_command += [ "-B", args.build_dir ]
    cmake_command += [ f"-DCMAKE_BUILD_TYPE={args.config}" ]
    cmake_command += [ f"-DCMAKE_TOOLCHAIN_FILE={cmake_toolchain_file}" ]
    cmake_command += [ f"-DCMAKE_ANDROID_NDK={args.ndk}" ]
    cmake_command += [ f"-DCMAKE_ANDROID_ARCH_ABI={args.abi}" ]
    cmake_command += [ f"-DCMAKE_ANDROID_STL_TYPE=c++_static" ]
    cmake_command += [ f"-DANDROID_NDK={args.ndk}" ]
    cmake_command += [ f"-DANDROID_ABI={args.abi}" ]
    cmake_command += [ f"-DANDROID_USE_LEGACY_TOOLCHAIN_FILE=NO" ]

    run_command( cmake_command )

def build( args ):
    run_command( [ "cmake",
        "--build", args.build_dir,
        "--config", args.config,
        "--parallel", args.parallel ] )

def install( args ):
    if args.install_dir:
        run_command( [ "cmake",
            "--install", args.build_dir,
            "--config", args.config,
            "--prefix", args.install_dir ] )

if __name__ == "__main__":
    args = parse_args()
    configure( args )
    build( args )
    install( args )
