#!/bin/bash

root_cwd="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" #
echo $root_cwd

echo "***** Building LLVM *****"
cd tdebug-llvm
make
source llvmvars.sh

echo "***** Building Task Debug Library *****"
cd $root_cwd
cd tdebug-lib
make clean
make
source tdvars.sh

echo "***** Building TBB *****"
cd $root_cwd
cd tbb-lib
make clean
make
source obj/tbbvars.sh
cd $root_cwd
