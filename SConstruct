#
# SConstruct for building WOML
#
# Usage:
#       scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] smartmet_woml.a|lib
#             [windows_boost_path=<path>]
#
# Notes:
#       The three variants (debug/release/profile) share the same output and 
#       object file names;
#       changing from one version to another will cause a full recompile.
#       This is intentional (instead of keeping, say, out/release/...)
#       for compatibility with existing test suite etc. and because normally
#       different machines are used only for debug/release/profile.

import os.path, os

Help(""" 
    Usage: scons [-j 4] [-Q] [debug=1|profile=1] [objdir=<path>] smartmet_frontier.a|lib
    
    Or just use 'make release|debug|profile', which point right back to us.
""") 

Decider("MD5-timestamp") 

DEBUG=      int( ARGUMENTS.get("debug", 0) ) != 0
PROFILE=    int( ARGUMENTS.get("profile", 0) ) != 0
RELEASE=    (not DEBUG) and (not PROFILE)     # default

OBJDIR=     ARGUMENTS.get("objdir","obj")

env = Environment( )

env.Append(LIBS=["smartmet_woml",
		 "smartmet_macgyver",
		 "smartmet_newbase",
		 "boost_date_time",
		 "boost_program_options",
		 "boost_regex",
		 "boost_iostreams",
		 "boost_filesystem",
		 "boost_thread",
		 "boost_system",
	 	 "bz2",
		 "pthread",
		 "xqilla",
		 "xerces-c",
		 "cairo",
		 "rt"
	  ]);

env.Append( CPPDEFINES= ["UNIX"] )
env.Append( CPPDEFINES="FMI_MULTITHREAD" )
env.Append( CPPDEFINES= "_REENTRANT" )

env.Append( CPPPATH= [ "./include", "/usr/include/smartmet/woml", "/usr/include/cairo" ] )

env.ParseConfig("(pkg-config --exists libconfig++ && pkg-config libconfig++ --cflags --libs)")

env.Append( CXXFLAGS= [
        # MAINFLAGS from orig. Makefile
        "-fPIC",
        "-Wall", 
        "-Wno-unused-parameter",
        "-Wno-variadic-macros",
	    
	    # DIFFICULTFLAGS from orig. Makefile
	    #
	    # These are flags that would be good, but give hard to bypass errors
	    #
	    #"-Wunreachable-code", gives errors if enabled (NFmiSaveBase.h:88)
	    #"-Weffc++",           gives errors if enabled (NFmiVoidPtrList.h: operator++)
	    #"-pedantic",          Boost errors (on OS X, Boost 1.35)
    ] )


if DEBUG:
	env.Append( CXXFLAGS=[ "-O0", "-g", "-Werror",
    
	# EXTRAFLAGS from orig. Makefile (for 'debug' target)
        #
        "-ansi",
        "-Wcast-align",
        "-Wcast-qual",
        "-Winline",
        "-Wno-multichar",
        "-Wno-pmf-conversions",
        "-Woverloaded-virtual",
        "-Wpointer-arith",
        "-Wredundant-decls",
        "-Wwrite-strings"
    
            # Disabled because of Boost 1.35
            #"-Wold-style-cast",
            #"-Wshadow",
            # "-Wsign-promo",
	    # Disabled because of RHEL6 nitpicking
	    # "-Wconversion",
        ] )

#
# Release settings
#
if RELEASE or PROFILE:
	env.Append( CPPDEFINES="NDEBUG",
        	    CXXFLAGS= ["-O2",
	            "-Wuninitialized",
        ] )


# Profile settings
#
if PROFILE:
	env.Append( CXXFLAGS="-g -pg" )


# Build library from source/*

lib_objs= []

for fn in Glob("source/*.cpp"): 
	s = os.path.basename( str(fn) )
	lib_obj_s = OBJDIR+"/"+ s.replace(".cpp","")
	lib_objs += env.Object( lib_obj_s, fn )

# Make just the static lib

env.Library( "smartmet_frontier", lib_objs )

# Build main programs

env.Program( "frontier", [ "frontier.cpp", "libsmartmet_frontier.a" ] );
