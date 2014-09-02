Mutantspider Makefile System

Audience: This document is mostly intended for people who want to _use_ the Mutantspider Makefile System.  For
detailed information on how it works, please see the file mutantspider.mk


The Mutantspider Makefile System is primarily implemented in the single file mutantspider.mk.  It is intended to be
used by including it in some other makefile, and then following a set of guidelines on how to write those other
makefiles that are using it.  It is also primarily intended to support a project that constucts a web application, in
part by cross-compiling C/C++ code into the various formats that are supported by modern web browsers -- for example
asm.js and Google's Native Client.  In this system, executing 'make' produces multiple executables all from the same
source files.

Currently (2014/2/26) the Mutantspider Makefile System supports Emscripten (->asm.js) and NaCl "pnacl" (->.pexe).

The most simplistic usage of MMS would be a Makefile that lived in the same directory as mutantspider.mk and looked
like:

#########################

# MMS guidelines are to build the target 'all' by defaut, so list it first here in case any of the further code
# wants to declare a target.  This will cause 'all' to be the default
.PHONY: all
all:

SOURCES:=  { some list of C/C++ source files you want to compile into your web page }
INC_DIRS:= { the include paths needed (if any) to compile the sources in SOURCES }

# this defines a set of functions we will use below. It is recommended to put "include mutantspider.mk" AFTER we
# have specified all of our SOURCES, compile options, etc...
include mutantspider.mk

#
# some example name we want to use for our application
#
BUILD_NAME:=myApp

#
# create build/dependency rules for all sources and all compilers
#
$(foreach src,$(SOURCES),$(eval $(call ms.COMPILE_RULE,$(src),$(INC_DIRS))))

#
# create build/dependency rules for all of the nacl targets
#
$(eval $(call ms.NACL_LINKER_RULE,$(BUILD_NAME),$(SOURCES)))

#
# create a build/dependency rule for the asm.js targets
#
$(eval $(call ms.EM_LINKER_RULE,$(BUILD_NAME),$(SOURCES)))

#
# cause the 'all' target to build all targets defined by mutantspider.mk
#
all: $(call ms.TARGET_LIST,$(BUILD_NAME))

###########################


With just this much, were you to cd to the directory containing this Makefile (and mutantspider.mk, because this
example has the two files in the same directory) and then type 'make' -- and assuming that your SOURCES all compiled
correctly in all of the compilers -- you would get a directory named 'out' in this same directory, and in there you
would have a directory named 'release', and in there you would get a set of myApp* files for each of the compilers
that MMS supports. Currently this set would be myApp.js, myApp.js.mem, myApp.pexe, and myApp.nmf.

If you actually run the experiment above you are likely to see several errors and warnings when you try to execute
make. mutantspider.mk requires, and checks for, certain things on your system.  If it finds something wrong with the
configuration it will tell you what is wrong and either stop, or just warn you that it is making a default decision
about something depending on what condition it is checking for.

Here are the REQUIREMENTS that mutantspider.mk checks for and stops if they are missing:
########################################################################################

1)  The directory containing mutantspider.mk must also have a directory named "nacl_sdk_root".  This must either be
    the root directory of whatever Pepper sdk (from Google's Native Client sdk) you want to use, or more realisticaly
    a symlink to that directory.  This means you need to have an appropriate NaCl sdk installed on your machine
    somewhere.
    
2)  Your machine must have the Emscripten tool set installed with 'emcc' available in the path.

Here are the things it will WARN about, but then make default decisions:
########################################################################

1)  If your Makefile does not define ms.INTERMEDIATE_DIR prior to including mutantspider.mk it will assume and use
    "obj" in the current directory.  Since the example above did not define this, you would see this warning if you
    used this Makefile
    
2)  If your Makefile does not define ms.OUT_DIR prior to including mutantspider.mk it will assume and use "out"
    in the current directory.  The Makefile above did not define this, so would have produced this warning, and it
    is also why the various myApp.* files are in the "out" directory.
    
There are also a few things that you can optionally set, but which have good-enough defaults that if you don't set
them MMS just uses its default.  These are:
######################################

1)  CONFIG.  If this is not set prior to including mutantspider.mk then it will default to "release".  That is why
    the example above produces its output in the "release" directory of "out".
    
2)  V (for "verbose output"). If not set, mutantspider.mk defaults to V=0.  When V=0, while make is processing it
    will show abreviated lines for what tool is currently running and what file it is working on.  When V is anything
    other than 0 it will show the full command lines that are executing.  For example, invoking make with "make V=1"
    on the command line will cause it to output full command lines.
    

A few cool things that MMS does:
################################

Dependency Tracking.  While not absolutely perfect in every possible situation, makefiles built with MMS will
generally only compile what needs to be compiled each time.  In addition to automatically tracking header file
dependencies it also tracks compiler and linker option changes.  If you change the compiler options you are using it
will recompile everything.

Supports multiple source files with the same name (for example, "utils.c"), as long as they are in different
directories.

Supports fully dependent makefiles so that parallel make works correctly.  Executing make like "make -j8" will
cause it to compile 8 files in parallel (speeding up build times if you have an 8 core machine).

Automatically provides reasonable compiler options for both "release" and "debug" builds for all of the compilers it
supports.

Makes it easy for you to define configurations other than just "release" and "debug".  In fact, it has a generic
mechanism for dealing with configurations, and uses this mechanism internally to define the settings for "debug" and
"release".


Defining (or customizing) configurations:
##########################################

Any single invocation of make in MMS uses a single "configuration".  This is simply the value of the CONFIG variable
for that invocation of make.  If not specified, CONFIG will default to "release".  The value of the CONFIG variable
affects the build in the following ways:

1)  Intermediate and final output files are placed in directory named $(CONFIG) -- within the $(ms.INTERMEDIATE_DIR)
    or $(ms.OUT_DIR) directories.
    
2)  Compiles are done using the compiler flags defined in the following variables:

        CFLAGS_$(CONFIG)
        CFLAGS_<compiler>_$(CONFIG)

    where <compiler> is the compiler being used for that compile (currently either "pnacl" or "emcc")
    
3)  Linking is done using the linker flags defined in the following variables:

        LDFLAGS_$(CONFIG)
        LDFLAGS_<linker>_$(CONFIG)

    where <linker> is the linker being used for that build (currently either "pnacl" or "emcc")
    
MMS provides reasonable defaults for the "release" and "debug" configurations by defining an appropriate set of these
CFLAGS_* and LDFLAGS_* variables so that the generic configuration mechanism ends up using approrpriate options for
these two configurations.  Its internal mechanism to specify these is done with "+=" syntax.  So, for example, it
contains statements that look like:

# add "-O2" to whatever else is in CFLAGS_release
CFLAGS_release += -O2

This permits your makefile to also specify certain options that you want used in certain configurations or with
certain compilers.


Creating "component-like" Makefiles
###################################

MMS is oriented towards a build system where all source files are compiled into a single executable.  It does not
have formal support for ideas like dll's and shared objects.  You can use them, but the MMS mechanisms don't provide
tools to automatically build them and link them together.  This is mostly because some of the compilers in the set
that it supports do not have good mechanisms for these sorts of objects.

The idea instead is that logical groups of source files would have a dedicated <component>.mk and that the primary
Makefile would be expected to include that file the same way it includes mutantspider.mk.

Here are some guidelines to attempt to follow when writing such a makefile:

1)  APPEND your sources to the $(SOURCES) variable.  This allows the primary Makefile to end up with a single
    $(SOURCES) variable that contains the union of all component sources.  For example:
    
    SOURCES+=\
    mySource1.cpp\
    mySource2.cpp\
    mySource3.cpp
    
2)  APPEND your custom include paths to the $(INC_DIRS) variable.  This should be done without the "-I" prefix used
    by most compilers.  For example:

    INC_DIRS+=\
    myDirectory1\
    myDirectory2

3)  Use relative path's to all of your sources and include dir's and specify them so that they are relative to the
    original make directory $(CURDIR) -- not simply relative to the directory of your makefile.  This will require a
    small bit of make-style computing to determine depending on the directory layout you are using for your
    component. In a simple case where all of your files are in known locations that are subdirectories of where your
    makefile is, you can do this by simply computing the directory of your makefile.
    
    Consider the following layout:
    
    component1_dir
        component1.mk
        source1.cpp
        sub_dir
            source2.cpp
            source2.h
            
    where indentation represents directory structure.  If source1.cpp #includes source2.h then it is likely that
    sub_dir will need to be included in INC_DIRS.  Both source1.cpp and source2.cpp need to be included in SOURCES.
    The following make syntax will let you do this:
    
    ##############################
    #
    # contents of component1.mk
    #
    
    #
    # set COMPONENT1_ROOT to the root of our directory structure (component1_dir) - RELATIVE to the original make
    # directory $(CURDIR)
    # gnu make documentation for 'dir', 'lastword' and 'MAKEFILE_LIST' can explain what this is doing
    COMPONENT1_ROOT:=$(dir $(lastword $(MAKEFILE_LIST)))
    
    # now these are easy
    SOURCES+=\
    $(COMPONENT1_DIR)source1.cpp\
    $(COMPONENT1_DIR)sub_dir/source2.cpp
    
    INC_DIRS+=\
    $(COMPONENT1_DIR)sub_dir
    
    ###############################
    
    Things get a little more complicated when the root of your directory is higher up than where your component.mk
    file lives. But this is also not too bad.
    
    Consider the following layout:
    
    component2_dir
        source
            source1.cpp
            source2.cpp
        include
            source1.h
            source2.h
        build
            component2.mk
            
    In this case we want to figure out the directory that is one up from where we found it in the first example.  And
    while this can be done by just appending '../' to the end of what we found, that ends up being a little messy in
    certain parts of MMS if there are multiple ways to specify a single directory that ends up being referenced.  For
    example dir1/dir2 is the same directory as dir1/dir2/dir3/..  But some of the object file handling in MMS will
    end up placing things in different directories unnecessarily if we don't simplify the representations of these
    directories.  Here is a bit of make magic to clean these things up.
    
    ##############################
    #
    # contents of component2.mk
    #
    
    #
    # computes the parent directory of $1, doing the right thing if $1 is itself a directory,
    # and in particular, if $1 is something like '../..'  It ASSUMES that $1 does NOT end with '/'
    #
    #	innermost substitution adds '/../foo' to the end of $1, if $1 ends with '..'  Next one adds
    #	'./foo' if it ends with '.'  Note that at most one of these two steps will add anything.
    #	The result of that is then processed with 'dir', to strip off whatever is now at the end.
    #	That will either be the 'foo' we added (leaving whatever '..' stuff we added),
    #	or will be the last real directory (or file) name.
    #
    #	Finally, we strip the trailing '/', which dir always adds and we don't want
    #
    parent_dir=$(patsubst %/,%,$(dir $(patsubst %.,%../foo,$(patsubst %..,%../../foo,$(1)))))

    #
    # set COMPONENT2_DIR to one directory up from the directory that contains this
    # makefile (component2.mk) - RELATIVE TO THE ORIGINAL MAKE DIRECTORY $(CURDIR).
    # The last word in MAKEFILE_LIST is our component2.mk relative to whatever
    # directory was current when make was first run. Done with 2 recursive calls to parent_dir
    #
    COMPONENT2_DIR := $(call parent_dir,$(call parent_dir,$(lastword $(MAKEFILE_LIST))))
    
    # now these are easy, and unlike the component1 example, parent_dir does _not_
    # leave the trailing '/', so we add it after each $(COMPONENT2_DIR)
    SOURCES+=\
    $(COMPONENT2_DIR)/source/source1.cpp\
    $(COMPONENT2_DIR)/source/source2.cpp
    
    INC_DIRS+=\
    $(COMPONENT2_DIR)/include
    
    ###############################
    
4)  If your component has special compile configurations that you use for certain, perhaps debugging, purposes you
    can easily add a configuration to the whole build system by just defining the compiler and linker options you
    need for that configuration.  For example, supposed your component has a set of additional defines that you want
    to use when compiling, you can just put this in your component.mk file:
    
    #
    # add configuration "eng" by describing its cflags
    #
    CFLAGS_eng=$(CFLAGS_debug) -DENGR_BUILD

    This will allow someone to invoke make with "make CONFIG=eng" and it will build all of the sources with the
    normal debug flags (becuase that is what we specified above), plus #defining ENGR_BUILD.  If you want to permit
    syntax that looks like:
    
        make eng
        
    where "eng" is a target, then you can add something like this to your component makefile:

    # when anyone tries to build the "eng" target, set CONFIG to "eng"
    ifeq (eng,$(MAKECMDGOALS))
    CONFIG=eng
    endif

    # make the "eng" target and force it to build the 'all' target -- MMS recommends that the default target
    # is 'all' so putting it as a prerequisite of 'eng' causes the 'eng' target to build the 'all' target
    # (with CONFIG=eng).
    .PHONY: eng
    eng: all
    
5)  And, in fact, the example above in 4 can mostly be done on the command line too, without modifying any makefile
    sources.  Assume you wanted to build a special version of your components that had "-msse4.2" added to the
    command line of all compiles.  Without modifying any of the makefiles, simply invoking make with:
    
    make CONFIG=sse4.2 CFLAGS_sse4.2="$(CFLAGS_release) -msse4.2"
    
    will cause it to produce its output in $(ms.OUT_DIR)/sse4.2/, and all files will be compiled with whatever is
    defined in CFLAGS_release plus the -msse4.2 flag.

6)  File Specific Compiler Flags:

    For each compiler supported MMS includes the variable:
    
    CFLAGS_<compiler-name>_<source-file-name>
    
    in the options that are passed to the compiler when compiling that file in that compiler.  To modify the
    component2 makefile above in 3, so that it added -msse4.2 to the compile options only for source2.cpp, when
    compiled for pnacl, the makefile could contain a statement such as:
    
    CFLAGS_pnacl_$(COMPONENT2_DIR)/source/source2.cpp = -msse4.2
    
    And like 5 above, this can also be done on the command line.
	

Resources:
##########

"Resources" refer to file-like things that your application may need at runtime.  MMS supports resources via the
$(RESOURCES) make variable.  Any filename that is listed in the RESOURCES variable at the time you execute 'make'
will be available in the /resources file system at runtime.  For example, assume that your application needs to read
the contents of a file in your component2_dir (from above) named "startup.conf", and it sits within a directory
named "rez" within the component2_dir.  If component2.mk contains a line like:

    RESOURCES+=$(COMPONENT2_DIR)/rez/startup.conf
	
then C/C++ code will be able to execute:

    FILE* f = fopen("/resources/startup.conf", "r");
	
to open and read the contents of that file.  All such files will appear flat within the "/resources" directory. This
directory is mounted read-only, and so it will fail if you attempt to add files to it or modify any of the existing
files.  Files within the "/resources" directory must be opened read-only ("r", if you are using fopen).  The
dependency mechanism of MMS will correctly re-include the startup.config file if you edit it and then execute make
again.

If you want a resource file to appear in a subdirectory of "/resources" then you can specify an additional make
variable, whose name is the (path-qualified) file name, plus "_DST_DIR".  The value of that variable, if it exists,
will be interpreted as the path of the directory in which you want the file to appear.  All such files will still
appear inside of "/resources", and so you do _not_ specify that.  For the example above, if component2.mk also
specified:

    $(COMPONENT2_DIR)/rez/startup.conf_DST_DIR = /<b>my_subdir</b>

then calls to:

    FILE* f = open("/resources/<b>my_subdir</b>/startup.conf", "r");

will succeed in opening the file, allowing you to read its contents.  Note that the <file>_DST_DIR definition starts
with a "/".  This is required.

You can have any number of these resource files, although each will increase the size of your web app, and so your
download, by the size of the resource file itself (plus some small amount of overhead).

Mutanspider.mk defines a special target named "display_rez" that will print out all of the resource files you are
using in the location that they will be available to fopen, along with the original source file that they come from.
Executing "make display_rez" will show you this list.  If you don't have any resources defined then it will tell you
that you don't have any resources.

NOTE: this mechanism in mutantspider.mk only works with the files defined it RESOURCES ---> at the time you include
mutantspder.mk from your makefile <---  If you add to RESOURCES after including mutantspider.mk, whatever you add
will not be included in the resource mechanism.  Your "include mutanspdider.mk" statement must come _after_ any
statements that set SOURCES, INC_DIRS, or RESOURCES.


A few more cool things you can do with MMS:
###########################################

1. Removing specific files from specific builds.

Your project may contain source files that are needed in your pnacl build, but are not compilable in your emcc build.
While you could solve this problem by adding #ifndef (EMSCRIPTEN) blocks inside the files themselves sometimes you
don't want to modify these sources at all.  When computing the list of files to compile for a particular build, MMS
always excludes any file that is listed in the <compiler>_EXCLUDE variable.  So for example:

emcc_EXCLUDE+=$(COMPONENT2_DIR)/source/source1.cpp

will cause source1.cpp to be skipped when building with emcc (but not skipped when building with pnacl).


2.  Google's Pepper sdk comes with both debug and release builds of their ppapi and ppapi_cpp libraries.  If you know
the difference between these two and want to specify which of the two are used for your build you can do this by
setting the ms.NACL_LIB_PATH to either Debug or Release (be careful to use these exact names, including the
captitalization). If not specified, MMS will use Debug for CONFIG=debug, and Release for anything else.


3.  If your project includes javascript files and you would like to run the closure compiler that comes with your
currently installed emscripten toolset, the variable ms.EMCC_CLOSURE_COMPILER is the full path to that jar file
for that version of closure.  For example, if your Makefile contains:

    java -jar $(ms.EMCC_CLOSURE_COMPILER) <various closure options>

as a recipe in one of your javascript-related targets then you can run this version of the closure compiler without
needing to install yet another one.

