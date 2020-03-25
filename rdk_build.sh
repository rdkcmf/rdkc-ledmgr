#!/bin/bash
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#######################################
#
# Build Framework standard script for
#
# xw4 common component
# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e

# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
export RDK_DIR=$RDK_PROJECT_ROOT_PATH
#source $RDK_SCRIPTS_PATH/soc/build/soc_env.sh

# To enable rtMessage
export RTMESSAGE=yes

if [ "$XCAM_MODEL" == "SCHC2" ]; then
. ${RDK_PROJECT_ROOT_PATH}/build/components/amba/sdk/setenv2
elif [ "$XCAM_MODEL" == "SERXW3" ] || [ "$XCAM_MODEL" == "SERICAM2" ]; then
. ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
else #No Matching platform
    echo "Source environment that include packages for your platform. The environment variables PROJ_PRERULE_MAK_FILE should refer to the platform s PreRule make"
fi

# parse arguments
INITIAL_ARGS=$@
function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -p    --platform  =PLATFORM   : specify platform for xw4 common"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}
# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvp: -l help,verbose,platform: -- "$@")
then
    usage
    exit 1
fi
eval set -- "$GETOPT"
while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -p | --platform ) CC_PLATFORM="$2" ; shift ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done
ARGS=$@
# component-specific vars
export FSROOT=${RDK_FSROOT_PATH}
export PLATFORM_SDK=$RDK_FSROOT_PATH

export NM=${RDK_TOOLCHAIN_PATH}/bin/mipsel-linux-nm
export RANLIB=${RDK_TOOLCHAIN_PATH}/bin/mipsel-linux-ranlib

export PATH="$PREFIX_FOLDER/bin:$PATH"

# functional modules
function configure()
{
    echo "Executing configure common code"
    true
}

function clean()
{
    echo "Start Clean"
    cd $RDK_SOURCE_PATH
    make clean
}
function build()
{
    cd ${RDK_SOURCE_PATH}
   echo "Building ledmgr common code"
   cd ${RDK_SOURCE_PATH}
   make
   install
}
function rebuild()
{
    clean
    configure
    build
}
function install()
{
    cd ${RDK_SOURCE_PATH}
    if [ -f "libledmgr.so" ]; then
       cp libledmgr.so ${RDK_FSROOT_PATH}/usr/lib
    fi

    if [ -f "ledtest" ]; then
	cp ledtest ${RDK_FSROOT_PATH}/usr/bin
    fi

    if [ -f "ledmgrmain" ]; then
       cp ledmgrmain ${RDK_FSROOT_PATH}/usr/bin
    fi
}
# run the logic
#these args are what left untouched after parse_args
HIT=false
for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done
# if not HIT do build by default
if ! $HIT; then
  build
fi
