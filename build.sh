#!/usr/bin/env bash
set -e # halt script on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# download the dependencies (universal-pddl-parser and VAL)
cd $SCRIPT_DIR
git submodule init
git submodule update

# build the universal-pddl-parser
cd $SCRIPT_DIR/universal-pddl-parser
scons

# build the temporal planning tools in the repository
cd $SCRIPT_DIR
scons

# build the domain transformer for the AllenAlgebra domain
cd $SCRIPT_DIR/domains/AllenAlgebra
scons

# build VAL
cd $SCRIPT_DIR/VAL
cmake CMakeLists.txt
make

