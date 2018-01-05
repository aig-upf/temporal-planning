import os
import glob
import sys
import platform

def which(program):
	""" Helper function emulating unix 'which' command """
	for path in os.environ["PATH"].split(os.pathsep):
		path = path.strip('"')
		exe_file = os.path.join(path, program)
		if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
			return exe_file
	return None

pkgpath = os.environ.get( "PKG_CONFIG_PATH", "" )
os.environ["PKG_CONFIG_PATH"] = pkgpath

src_path = "./bin"

# Read the preferred compiler from the environment - if none specified, choose CLANG if possible
#default_compiler = 'clang++' if which("clang++") else 'g++'
default_compiler = 'g++'
gcc = os.environ.get('CXX', default_compiler)

base = Environment(tools=["default"], CXX=gcc)

base['pddl_parser_path'] = os.path.abspath(os.environ.get('PDDL_PARSER_PATH', '../universal-pddl-parser/'))

include_paths = ['.', base['pddl_parser_path']]

if platform.system() == "Darwin":
	base.Append(LINKFLAGS=['-undefined', 'dynamic_lookup'])

base.AppendUnique(
	CPPPATH = [ os.path.abspath(p) for p in include_paths ],
	CXXFLAGS=["-Wall", "-pedantic", "-std=c++11", "-g", "-Wno-long-long"]
)

base.Append(LIBS=[File(os.path.abspath('../universal-pddl-parser/lib/libparser.a'))])

# The compilation of the (static & dynamic) library
build_dirname = 'build'
base.VariantDir(build_dirname, '.')

sources = glob.glob( src_path + "/*.cpp" )
build_files = [build_dirname + '/' + src for src in sources]

compileSHE = base.Program( "bin/compileSHE", ["bin/compileSHE.cpp"] )
compileTempo = base.Program( "bin/compileTempo", ["bin/compileTempo.cpp"] )
compileTempoParallel = base.Program( "bin/compileTempoParallel", ["bin/compileTempoParallel.cpp"] )
compileSequential = base.Program( "bin/compileSequential", ["bin/compileSequential.cpp"] )
#compileFull = base.Program( "bin/compileFull", ["bin/compileFull.cpp"] )
planSchedule = base.Program( "bin/planSchedule", ["bin/planSchedule.cpp"] )
#testSeparable = base.Program( "bin/testSeparable", ["bin/testSeparable.cpp"] )

base.AlwaysBuild( compileSHE )
base.AlwaysBuild( compileTempo )
base.AlwaysBuild( compileTempoParallel )
base.AlwaysBuild( compileSequential )
#base.AlwaysBuild( compileFull )
base.AlwaysBuild( planSchedule )
#base.AlwaysBuild( testSeparable )
