#
#	MutantSpider core makefile.  See README.makefile for basic documentation
#
#	Some naming conventions:
#		1) variables defined by MutantSpider will be in the ms "namespace".  That is, they will be of the form
#			ms.VariableName
#		2) exceptions to #1 include:
#			a) variables that are reasonable to type on the command line, for example "V" and "CONFIG".
#			b) the collection of compiler and linker flags (CFLAGS, CFLAGS_release, etc...).
#		3) variable names in ms of the form ms.ALL_CAPS_AND_UNDERSCORES are expected to be "public"
#			in the sense that code outside of mutantspider.mk can reasonably expect to use and manipulate those.
#		4) variable names in ms of the form ms.lower_case are expected to be "private" and should not.
#			be used outside of mutantspider.mk
#
#	TODO list:
#		1) provide explicit bindings of source file suffix -> compiler, and then use $(filter ...) to figure
#			out which files need to be expanded with which build rules
#		2) change configuration validation (installed compilers, etc...) so that it is only triggered if
#			you attempt to build something with that compiler.  Once #1 is done, if *.ts -> the typescript compiler
#			then it should only check to make sure you have the typescript compiler installed if your SOURCES
#			contain *.ts files.  This can be done similar to the way the dir.stamp files are handled.
#		3) ms.do_strip is currently only running if the target is exactly "release".  Improve this somehow.


#
# the directory that this makefile (mutantspider.mk) is in -- relative to the orignal
# directory in which make was invoked $(CURDIR)
#
ms.this_make_dir:=$(dir $(lastword $(MAKEFILE_LIST)))

#
# Make sure there is a nacl_sdk_root symlink directory
#
ifeq (,$(wildcard $(ms.this_make_dir)nacl_sdk_root))
 $(info *********************************)
 $(info 'nacl_sdk_root' directory missing)
 $(info Please create a symbolic link named 'nacl_sdk_root' in $(realpath $(ms.this_make_dir)),)
 $(info pointing to the NaCl/Pepper sdk directory you want to use.  For information on how to)
 $(info install the NaCl SDK, please google search for "Google Native Client SDK Download")
 $(error )
endif

#
# Make sure the nacl_sdk_root directory looks like it is pointing to something reasonable
#
ifeq (,$(wildcard $(ms.this_make_dir)nacl_sdk_root/tools/oshelpers.py))
 $(info *********************************)
 $(info nacl_sdk_root is set to an invalid location: nacl_sdk_root -> $(realpath $(ms.this_make_dir)nacl_sdk_root))
 $(info which does not appear to contain the right files)
 ifneq (,$(wildcard $(ms.this_make_dir)nacl_sdk_root/pepper_*))
  $(info Did you mean to point to one of these? -> $(foreach sdk,$(wildcard $(ms.this_make_dir)nacl_sdk_root/pepper_*),$(realpath $(sdk))))
 endif
 $(error )
endif

#
# Make sure emcc is installed and in the current path
#
ifeq (,$(shell which emcc))
 $(info *********************************)
 $(info Emscripten compiler 'emcc' is either not installed or not available in the current path)
 $(info ("which emcc" failed to find it).  For information on how to install the emscripten SDK)
 $(info please google search for "Emscripten Download".  For information on how to get a Macintosh)
 $(info terminal window to automatically run the "source emsdk_add_path" tool suggested when you)
 $(info run "emsdk activate latest", google search for "bashrc equivalent mac" -- you will see)
 $(info information on using your .profile or .bash_profile file instead.  The emscripten)
 $(info compiler may require that you have 'python2' installed as a callable tool.  Macs)
 $(info don't normally come installed with that.  One workable solution is to create a symbolic)
 $(info link in /usr/bin/python2 that points to the normally installed /usr/bin/python, using)
 $(info a command line "sudo ln -s /usr/bin/python /usr/bin/python2" -- assuming that your)
 $(info python is installed at /usr/bin/python -- which you can see by executing)
 $(info "which python".  Note that the very first time you run the emscripten compiler on)
 $(info your system it will create various caches and run very slowly.  Be patient!)
 $(info )
 $(info now...)
 $(info )
 $(info If you want or need to carefully control the web development tools on your system)
 $(info you should be aware that running "source emsdk_add_path" puts the emscripten bin path)
 $(info ahead of places like /usr/bin, and these emscripten tools come with their own version)
 $(info of a few tools like 'node' and 'npm'.  So any shell that sources emsdk_add_path and)
 $(info then tries to execute an undecorated 'node', will be running the version from your)
 $(info emscript sdk.  In a similar fashion, running 'npm' with -g will place the installed)
 $(info tools in your emscripten 'node/<version>/bin' directory.  This may, or may not be what)
 $(info you want.  For anyone who is not picky about where these tools are installed, or what)
 $(info version is installed, letting them install in this location could be a simple solution)
 $(error )
endif

#
# Root directory into which temp .o files will be placed
#
ifeq (,$(ms.INTERMEDIATE_DIR))
 ms.INTERMEDIATE_DIR:=obj
 $(info ms.INTERMEDIATE_DIR not set, so defaulting to $(CURDIR)/$(ms.INTERMEDIATE_DIR))
 $(info To remove this message set ms.INTERMEDIATE_DIR = something prior to including mutantspider.mk)
endif

#
# Root directory into which final executables will be placed
#
ifeq (,$(ms.OUT_DIR))
 ms.OUT_DIR:=out
 $(info ms.OUT_DIR not set, so defaulting to $(CURDIR)/$(ms.OUT_DIR))
 $(info To remove this message set ms.OUT_DIR = something prior to including mutantspider.mk)
endif

#
# Default configuration is 'release'
#
CONFIG?=release

#
# default value for "verbose output" is off
#
V?=0

#
# a few tools that we are silent about when verbose is off
#
ifneq (0,$(V))
 ms.mkdir=mkdir
 ms.touch=touch
 ms.sed=sed
else
 ms.mkdir=@mkdir
 ms.touch=@touch
 ms.sed=@sed
endif

#
# the full path to the closure compiler that is associated with the current emcc
#
ms.EMCC_CLOSURE_COMPILER := $(shell which emcc | sed 's:\(^.*\)emcc$$:\1third_party/closure-compiler/compiler.jar:')

###############################################################

# using a few tools from the nacl sdk

ms.getos := python $(ms.this_make_dir)nacl_sdk_root/tools/getos.py
ms.osname := $(shell $(ms.getos))
ms.tc_path := $(realpath $(ms.this_make_dir)nacl_sdk_root/toolchain)
ms.lib_root := $(ms.this_make_dir)nacl_sdk_root/lib/pnacl

ms.pnacl_tc_bin := $(ms.tc_path)/$(ms.osname)_pnacl/bin
ms.pnacl_cc := $(ms.pnacl_tc_bin)/pnacl-clang
ms.pnacl_cxx := $(ms.pnacl_tc_bin)/pnacl-clang++
ms.pnacl_link := $(ms.pnacl_tc_bin)/pnacl-clang++
ms.pnacl_ar := $(ms.pnacl_tc_bin)/pnacl-ar
ms.pnacl_finalize := $(ms.pnacl_tc_bin)/pnacl-finalize
ms.pnacl_translate := $(ms.pnacl_tc_bin)/pnacl-translate
ms.pnacl_strip := $(ms.pnacl_tc_bin)/pnacl-strip

#
# should we include the Debug or Release lib path?
#
ifeq (debug,$(CONFIG))
 ms.NACL_LIB_PATH ?= Debug
else
 ms.NACL_LIB_PATH ?= Release
endif

#
# the emscripten tool(s)
#
ms.em_cc := emcc
ms.em_cxx := emcc
ms.em_link := emcc

#
# Strategy for only calling mkdir on output directories once.
# We put a file named "dir.stamp" in each of those directories
# and then put an "order only" prerequesite to this file in the
# list of each file to compile.
#
%dir.stamp :
	$(ms.mkdir) -p $(@D)
	$(ms.touch) $@

#
# Convert a source path to an object file path.
#
# $1 = Source Name
# $2 = Compiler suffix
#
ms.src_to_obj=$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(basename $(subst ..,__,$(1)))$(2).o

#
# Convert a source path to a dependency file path.
#
# $1 = Source Name
# $2 = Compiler suffix
#
ms.src_to_dep=$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(basename $(subst ..,__,$(1)))$(2).d

#
# Convert a (re)source path to it's equivalent auto_gen file
#
ms.resrc_to_auto_gen=$(ms.INTERMEDIATE_DIR)/auto_gen/$(subst ..,__,$(1)).cpp

#
# Tool to either display short-form (when verbose is off) or
# full command line (when verbose is on) for some tool being
# run as part of a recipe.
#
# $1 = (fully path-qualified, if necessary) tool to run
# $2 = arguments passed to that tool
# $3 = short display-form of the arguments -- frequently just the target file name
#
ifneq (0,$(V))
 ms.CALL_TOOL=$(1) $(2)
else
 ms.CALL_TOOL=@printf "%-17s %s\n" $(notdir $(1)) $(3) && $(1) $(2)
endif

#
# call the symbol stripping tool either with full echoing (when verbose is on)
# or silently (when verbose is off).  This function strips to a ".strip" file
# and then replaces the original with the .strip file.
#
# $1 = (fully path-qualified, if necessary) name of strip tool
# $2 = name of file to be stripped
#
ifeq (release,$(CONFIG))
 ms.do_strip = $(call ms.CALL_TOOL,$(1),-o $(2).strip $(2),$(2)) && rm $(2) && mv $(2).strip $(2)
endif

#
# compiler flags that are used in release and debug builds for all compilers
#
CFLAGS_release+=-O2 -DNDEBUG
# REVISIT : Should be -O0 for debug but that is making some binaries too big.
CFLAGS_debug+=-g

#
# something in DEBUG stuff causes link failures in emcc (at least version 1.16)
# so we set this macro to release-style there.
#
CFLAGS_pnacl_debug+=-D_DEBUG=1
CFLAGS_emcc_debug+=-DNDEBUG

#
# compiler and linker flags that are used in release emcc
# TODO: check on --closure 1
#
CFLAGS_emcc_release+=-s FORCE_ALIGNED_MEMORY=1 -O2 -s DOUBLE_MODE=0 -s DISABLE_EXCEPTION_CATCHING=0 --llvm-lto 1
LDFLAGS_emcc_release+=-s FORCE_ALIGNED_MEMORY=1 -O2 -s DOUBLE_MODE=0 -s DISABLE_EXCEPTION_CATCHING=0 --llvm-lto 1 --closure 0

CFLAGS_emcc_debug+=-s ASSERTIONS=1
LDFLAGS_emcc_debug+=-s ASSERTIONS=1

#
# If your build needs some additional symbols exported in the emcc build, you can add them by defining them
# in ms.EM_EXPORTS
#
ms.EM_EXPORTS+='_MS_Init', '_MS_MouseProc', '_MS_FocusProc', '_MS_KeyProc', '_MS_DidChangeView', '_MS_TouchProc', 'Pointer_stringify', '_MS_MessageProc', '_MS_DoCallbackProc', '_MS_SetLocale', '_main'

#
# the projects we are interested produce smaller files if memory-init-file is turned on.  We also add some standard asm.js library stuff
#
CFLAGS_emcc+=--memory-init-file 1
LDFLAGS_emcc+=--memory-init-file 1 -s EXPORTED_FUNCTIONS="[$(ms.EM_EXPORTS)]" --js-library $(ms.this_make_dir)library_mutantspider.js --js-library $(ms.this_make_dir)library_rezfs.js

#
# This filesystem implementation will hopefully, eventually get merged into emscripten
# itself.  For now we use our own copy, and so add this to the link
#
LDFLAGS_emcc+=--js-library $(ms.this_make_dir)library_pbmemfs.js

#
# a few files that implement some (mostly emscripten) support code
#
SOURCES+=\
$(ms.this_make_dir)mutantspider.cpp\
$(ms.this_make_dir)mutantspider_fs.cpp

#
# everyone will need to #include "mutantspider.h"
#
INC_DIRS+=$(ms.this_make_dir)

#
# before the compiler options check logic
#
ifneq (,$(RESOURCES))
CFLAGS+=-DMUTANTSPIDER_HAS_RESOURCES
endif


ifneq (clean,$(MAKECMDGOALS))
#
# helper function to compare current options to previously used options
#
#	$1 file name
#	$2 options
#
# Basic idea is to echo the current options, $(2), into the file $(1).  Then compare
# the contents of $(1) with the existing $(1).opts file.  If $(1).opts is missing or is
# not the same as $(1) then replace $(1).opts with $(1).  This has the effect of updating
# the time stamp on $(1).opts whenever the options change, making $(1).opts suitable for a
# prerequisite file to something that uses the options in $2
#
ms.d:=$(ms.INTERMEDIATE_DIR)/$(CONFIG)
ms.options_check=\
mkdir -p $(ms.d);\
echo "$(subst ",\",$(2))" > $(ms.d)/$(1);\
if [ -a $(ms.d)/$(1).opts ]; then\
	if [ "`diff $(ms.d)/$(1).opts $(ms.d)/$(1)`" != "" ]; then\
		echo "updating          $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).opts";\
		mv $(ms.d)/$(1) $(ms.d)/$(1).opts;\
	else\
		rm $(ms.d)/$(1);\
	fi;\
else\
	echo "creating          $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).opts";\
	mv $(ms.d)/$(1) $(ms.d)/$(1).opts;\
fi


#
# execute this shell code when this makefile is parsed.  We can't really do the
# option compare logic as part of a prerequisite of some target because we want to always
# perform the 'diff' operation, which would mean we would need to do something like
# make the target a .PHONY.  But then that would cause make to think it always needed
# to rebuild the target - even if our ms.options_check logic didn't end up updating
# the compiler.opts file.
#
ms.m:=$(shell $(call ms.options_check,compiler_pnacl,$(CFLAGS) $(CFLAGS_pnacl) $(CFLAGS_$(CONFIG)) $(CFLAGS_pnacl_$(CONFIG))))
ifneq (,$(ms.m))
$(info $(ms.m))
endif
ms.m:=$(shell $(call ms.options_check,compiler_emcc,$(CFLAGS) $(CFLAGS_$(CONFIG)) $(CFLAGS_emcc) $(CFLAGS_emcc_$(CONFIG))))
ifneq (,$(ms.m))
$(info $(ms.m))
endif
ms.m:=$(shell $(call ms.options_check,linker_pnacl,$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_pnacl) $(LDFLAGS_pnacl_$(CONFIG))))
ifneq (,$(ms.m))
$(info $(ms.m))
endif
ms.m:=$(shell $(call ms.options_check,linker_emcc,$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_emcc) $(LDFLAGS_emcc_$(CONFIG))))
ifneq (,$(ms.m))
$(info $(ms.m))
endif

# end of "if MAKECMDGOALS != clean"
endif

#
# helper target to see what the compilers and options are set to
#
.PHONY: display_opts
display_opts:
	@echo
	@echo "**** C/C++, pnacl ****"
	@echo $(realpath $(ms.pnacl_cxx))
	@cat $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_pnacl.opts
	@echo
	@echo "**** C/C++, emcc ****"
	@echo $(shell which emcc)
	@cat $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_emcc.opts
	@echo


#
# Compile Macro(s)
#
#	$1 = Souce Name
#	$2 = (with -I prefix) include directories
#
define ms.c_compile_rule
$(call ms.src_to_obj,$(1),_pnacl): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_pnacl.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.CALL_TOOL,$(ms.pnacl_cc),-o $$@ -c $$< -MD -MF $(call ms.src_to_dep,$(1),_pnacl) -I$(ms.this_make_dir)nacl_sdk_root/include $(2) $(CFLAGS) $(CFLAGS_pnacl) $(CFLAGS_$(CONFIG)) $(CFLAGS_pnacl_$(CONFIG)) $(CFLAGS_pnacl_$(1)),$$@)

$(call ms.src_to_obj,$(1),_js): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_emcc.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.CALL_TOOL,$(ms.em_cc),-o $$@ $$< -MD -MF $(call ms.src_to_dep,$(1),_js)d $(2) $(CFLAGS) $(CFLAGS_$(CONFIG)) $(CFLAGS_emcc) $(CFLAGS_emcc_$(CONFIG)) $(CFLAGS_emcc_$(1)),$$@)
	$(ms.sed) '1 c\'$$$$'\n''$(call ms.src_to_obj,$(1),_js): \\'$$$$'\n' $(call ms.src_to_dep,$(1),_js)d > $(call ms.src_to_dep,$(1),_js)
	@rm $(call ms.src_to_dep,$(1),_js)d

endef

define ms.cxx_compile_rule
$(call ms.src_to_obj,$(1),_pnacl): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_pnacl.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.CALL_TOOL,$(ms.pnacl_cxx),-o $$@ -c $$< -MD -MF $(call ms.src_to_dep,$(1),_pnacl) -I$(ms.this_make_dir)nacl_sdk_root/include $(2) -std=gnu++11 $(CFLAGS) $(CFLAGS_pnacl) $(CFLAGS_$(CONFIG)) $(CFLAGS_pnacl_$(CONFIG)) $(CFLAGS_pnacl_$(1)),$$@)

$(call ms.src_to_obj,$(1),_js): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_emcc.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.CALL_TOOL,$(ms.em_cxx),-o $$@ $$< -MD -MF $(call ms.src_to_dep,$(1),_js)d $(2) -std=c++0x $(CFLAGS) $(CFLAGS_$(CONFIG)) $(CFLAGS_emcc) $(CFLAGS_emcc_$(CONFIG)) $(CFLAGS_emcc_$(1)),$$@)
	$(ms.sed) '1 c\'$$$$'\n''$(call ms.src_to_obj,$(1),_js): \\'$$$$'\n' $(call ms.src_to_dep,$(1),_js)d > $(call ms.src_to_dep,$(1),_js)
	@rm $(call ms.src_to_dep,$(1),_js)d

endef

#
# Primary function that the top-most Makefile eval/calls to generate build/dependency rules
# for all of the sources it knows about
#
# for example:
#	$(foreach src,$(SOURCES),$(eval $(call ms.COMPILE_RULE,$(src),$(INC_DIRS))))
#
# assuming that SOURCES is a list of all sources and INC_DIRS is a list of all include directories
#
# $1 = source name
# $2 = (without -I prefix) include directories
#
define ms.COMPILE_RULE
ifneq (clean,$(MAKECMDGOALS))
-include $(call ms.src_to_dep,$(1),_pnacl)
-include $(call ms.src_to_dep,$(1),_js)
ifeq ($(suffix $(1)),.c)
$(call ms.c_compile_rule,$(1),$(foreach inc,$(2),-I$(inc)))
else
$(call ms.cxx_compile_rule,$(1),$(foreach inc,$(2),-I$(inc)))
endif
endif
endef

#
# at least currently, pnacl is easier to debug if we build a native nexe instead of a portable pexe
# ms.DO_NEXE can be set to force this to happen, but by default we don't do this.
#
ms.DO_NEXE?=0

#
#	$1	unprefixed and unpostfixed final executable name
#	$2	list of source files to compile and link
#	$3	additional libraries to link (will be listed last on the link line)
#
#	Note that the these targets will ignore any source file that is included in $(pnacl_EXCLUDE)
#	Also, the pnacl linker ends up attempting to open all input files simultaneously, and for projects with lots of
#	input files this can overflow several maximum open file limits in the system.  To address that problem this rule
#	first builds a lib out of the list of .o files and then passes that lib, instead of the .o's, to the linker.  But
#	that causes its own problems where the linker now uses lib symbol ordering rules instead of .o symbol ordering rules.
#	Since the nacl libs that we need to link to contain both cases where a nacl library must link to a symbol that is
#	defined in one of our .o's (which is now in our .lib), as well as many cases where one of our .o's needs to link
#	to a symbol in one of the nacl libraries, we solve that by providing the nacl libraries twice, once before and once
#	after our library.
#
ifneq (0,$(ms.DO_NEXE))
define ms.NACL_LINKER_RULE
$(ms.OUT_DIR)/$(CONFIG)/$(1).nexe: $(ms.INTERMEDIATE_DIR)/$(CONFIG)/linker_pnacl.opts $(foreach src,$(filter-out $(pnacl_EXCLUDE),$(2)),$(call ms.src_to_obj,$(src),_pnacl))
	@rm -f $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a
	$(call ms.CALL_TOOL,$(ms.pnacl_ar), -cr $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a $$(filter-out %.opts,$$^),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a)
	$(ms.mkdir) -p $$(@D)
	$(call ms.CALL_TOOL,$(ms.pnacl_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_pnacl) $(LDFLAGS_pnacl_$(CONFIG)) -L$(ms.lib_root)/$(ms.NACL_LIB_PATH) -L$(ms.INTERMEDIATE_DIR)/$(CONFIG) -o $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc -lppapi -lppapi_cpp -l$(1) -lppapi -lppapi_cpp -lnacl_io $(foreach lib,$(3),-l$(lib)),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc)
	$(call ms.CALL_TOOL,$(ms.pnacl_translate), --allow-llvm-bitcode-input -arch i686 $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc -o $$@, $$@)

endef
else
define ms.NACL_LINKER_RULE
$(ms.OUT_DIR)/$(CONFIG)/$(1).pexe: $(ms.INTERMEDIATE_DIR)/$(CONFIG)/linker_pnacl.opts $(foreach src,$(filter-out $(pnacl_EXCLUDE),$(2)),$(call ms.src_to_obj,$(src),_pnacl))
	@rm -f $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a
	$(call ms.CALL_TOOL,$(ms.pnacl_ar), -cr $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a $$(filter-out %.opts,$$^),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a)
	$(ms.mkdir) -p $$(@D)
	$(call ms.CALL_TOOL,$(ms.pnacl_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_pnacl) $(LDFLAGS_pnacl_$(CONFIG)) -L$(ms.lib_root)/$(ms.NACL_LIB_PATH) -L$(ms.INTERMEDIATE_DIR)/$(CONFIG) -o $$@ -lppapi -lppapi_cpp -l$(1) -lppapi -lppapi_cpp -lnacl_io $(foreach lib,$(3),-l$(lib)),$(ms.OUT_DIR)/$(CONFIG)/$(1).pexe)
	$(call ms.do_strip,$(ms.pnacl_strip),$$@)
	$(call ms.CALL_TOOL,$(ms.pnacl_finalize),$$@,$$@)

endef
endif

#
#	$1	unprefixed and unpostfixed final executable name
#	$2	list of source files to compile and link
#
#	Note that the this target will ignore any source file that is included in $(emcc_EXCLUDE)
#
define ms.EM_LINKER_RULE
$(ms.OUT_DIR)/$(CONFIG)/$(1).js: $(ms.INTERMEDIATE_DIR)/$(CONFIG)/linker_emcc.opts $(foreach src,$(filter-out $(emcc_EXCLUDE),$(2)),$(call ms.src_to_obj,$(src),_js))
	$(ms.mkdir) -p $$(@D)
	$(call ms.CALL_TOOL,$(ms.em_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_emcc) $(LDFLAGS_emcc_$(CONFIG)) -o $$@ $$(filter-out %.opts,$$^),$(ms.OUT_DIR)/$(CONFIG)/$(1).js)

#
# bug in gnumake???
# the rule to build the .js target also builds the .js.mem (this target)
# so we shouldn't really need any recipe here, and just an empty recipe _does_
# cause something like "make <mycomponent>.js.mem" it to build the file (by
# executing the recipe above for producing the .js).  But for some reason I can't
# figure out, later recipies in targets that have this .js.mem as a dependency
# don't run if the full list of targets also includes the .js file, and if this
# recipe is empty.  So we just execute a practically-do-nothing "touch" here as
# the recipe.  This is enough to get the later recipes to run.  This "touch" is
# _not_ about creating the file.
# ???
#
$(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem: $(ms.OUT_DIR)/$(CONFIG)/$(1).js
	@touch $(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem

endef

###############################################################################################
#
#   Resource Handling -- only evaluaged if $(RESOURCES) contains at least one file
#
###############################################################################################

ifneq (,$(RESOURCES))

#
#	$1 the path/file name to "sanitize" (make it a legal C token)
#
#		rules:
#			X	-> XX
#			.	-> X_
#           /   -> XS
#
ms.sanitize_rez_name=rezRec_$(subst /,XS,$(subst .,X_,$(subst X,XX,$(1))))

#
# $1 file name of path/file to be treated as a resource
#
define ms.resource_rule
$(call ms.resrc_to_auto_gen,$(1)): $(1) | $(dir $(call ms.resrc_to_auto_gen,$(1)))dir.stamp
	@echo "// AUTO-GENERATED by mutantspider.mk, based on the contents of $(notdir $(1))" > $$@
	@echo "// DO NOT EDIT" >> $$@
	@echo "" >> $$@
	@echo "#include <mutantspider.h>" >> $$@
	@echo "" >> $$@
	@echo "namespace mutantspider {" >> $$@
	@printf "const unsigned char $(call ms.sanitize_rez_name,$(1))_[" >> $$@
	@printf "`stat -f %z $(1)`" >> $$@
	@echo "] = {" >> $$@
	@cat $$< | od -vt x1 -An | sed 's/\(\ \)\([0-9a-f][0-9a-f]\)/0x\2,/g' >> $$@
	@echo "};" >> $$@
	@echo "extern const rez_file_ent $(call ms.sanitize_rez_name,$(1));" >> $$@
	@echo "const rez_file_ent $(call ms.sanitize_rez_name,$(1)) = { &$(call ms.sanitize_rez_name,$(1))_[0], `stat -f %z $(1)` };" >> $$@
	@echo "}" >> $$@

endef

#
# establish build targets of all of the resource C++ files
#
$(foreach rez,$(RESOURCES),$(eval $(call ms.resource_rule,$(rez))))

ms.rez_decl=$(foreach rez,$(RESOURCES),extern const rez_file_ent $(call ms.sanitize_rez_name,$(rez));)
ms.rez_files=$(foreach rez,$(RESOURCES),$(call ms.resrc_to_auto_gen,$(rez)))

#
# this list of all directories listed as parent of any RESOURCE file
#
ms.rez_dirs=$(sort $(foreach rez,$(RESOURCES),$($(rez)_DST_DIR)))

#
# some crazy helper functions
#
ms.reverse_1 = $(if $(1),$(call ms.reverse_1,$(wordlist 2,$(words $(1)),$(1)))) $(firstword $(1))
ms.reverse = $(strip $(call ms.reverse_1,$(1)))
ms.reverse_s1=$(if $(1),$(call ms.reverse_s1,$(wordlist 2,$(words $(1)),$(1))))/$(firstword $(1))
ms.reverse_s2=$(patsubst //%,/%,$(call ms.reverse_s1,$(1)))
ms.reverse_s=$(filter-out /,$(call ms.reverse_s2,$(1)))
ms.parent_dir_helper=$(call ms.reverse_s,$(wordlist 2,$(words $(1)),$(1)))
ms.parent_dir=$(call ms.parent_dir_helper,$(call ms.reverse,$(subst /, ,$(1))))

#
# helper
#
ms.dir_init={\"$(notdir $(1))\"{{COMMA}}{(rez_file_ent*)&$(call ms.sanitize_rez_name,$(1))XS}{{COMMA}}true}{{COMMAN}}

#
# If this directory, $(1), has a parent, then add an initialization snippet referencing
# this directory (ms.dir_init,$(1)), to the parent's initialization list.  If not, then
# add this snipped to the root initializer.  Regardless of whether $(1) has a parent
# or not, add the name of this directory's initialization list to ms.initializer_list
#
define ms.add_dir_initializer
$(if $(call ms.parent_dir,$(1)),ms.initializer_$(call ms.sanitize_rez_name,$(call ms.parent_dir,$(1)))XS+=$(call ms.dir_init,$(1)),ms.root_initializer+=$(call ms.dir_init,$(1)))
ms.initializer_list+=ms.initializer_$(call ms.sanitize_rez_name,$(1))XS
$(if $(call ms.parent_dir,$(1)),$(call ms.add_dir_initializer,$(call ms.parent_dir,$(1))))
endef

#
# loop through all of the resource directories, calling ms.add_dir_initializer
# on each one, this can produce repeated initializer snippets in some of the
# initializer lists because, for example:
#
#   foo/bar/dir1
#   foo/bar/dir2
#
# will both end up putting bar's snippet in foo's list.  So we finish by
# calling 'sort' on the list.  This removes the duplicates.  Note that because
# of this sort call, it is important that the snippets added by ms.add_dir_initializer
# do not have any spaces in them.
#
$(eval $(foreach rezd,$(ms.rez_dirs),$(call ms.add_dir_initializer,$(rezd))))
ms.initializer_list:=$(call ms.reverse,$(sort $(ms.initializer_list)))
ms.root_initializer:=$(sort $(ms.root_initializer))

#
# helper
#
ms.file_init={\"$(notdir $(1))\"{{COMMA}}{&$(call ms.sanitize_rez_name,$(1))}{{COMMA}}false}{{COMMAN}}

#
# Add an initialization snippet, referencing this file - $(1) - to
# this file's parent's initialization list
#
define ms.add_file_initializer
$(if $($(1)_DST_DIR),ms.initializer_$(call ms.sanitize_rez_name,$($(1)_DST_DIR))XS+=$(call ms.file_init,$(1)),ms.root_initializer+=$(call ms.file_init,$(1)))
endef

$(foreach rez,$(RESOURCES),$(eval $(call ms.add_file_initializer,$(rez))))

#
# finally, ms.dir_init_code will contain all of the C/C++ code that initializes
# the directory structures -- other than, it will have a set of {{xx}} tokens in
# it that will need to be swapped out for legal C characters.  These are used for
# a variety of reasons.  But one is that some places that would otherwise need to
# add a ',' character are in places where make itself would attempt to intperpret
# that character as meaningful in the expression.  So ','  ','-followed by newline,
# and newline are all written in as symbols for later substitution.
#
ms.construct_init_code=const rez_dir_ent $(2)_ents[] = {{{NL}}$(1)};{{NL}}rez_dir $(2) = {$(words $(1)){{COMMA}}&$(2)_ents[0]};

ms.dir_init_code:=$(foreach init,$(ms.initializer_list),$(call ms.construct_init_code,$($(init)),$(subst ms.initializer_,,$(init))))


###############

$(ms.INTERMEDIATE_DIR)/auto_gen/resource_list.cpp: $(RESOURCES)
	@echo "// AUTO-GENERATED by mutantspider.mk, based on the value of the make variable RESOURCES" > $@
	@echo "// DO NOT EDIT" >> $@
	@echo "" >> $@
	@echo "#include <mutantspider.h>" >> $@
	@echo "" >> $@
	@echo "namespace mutantspider {" >> $@
	@echo "" >> $@
	@echo "$(ms.rez_decl)" | sed 's/; /;/g' | sed 's/;/;\'$$'\n/g' >> $@
	@echo "" >> $@
	@echo "$(ms.dir_init_code)" | sed 's/; /;/g' | sed 's/{{COMMA}}/,/g' | sed 's/{{COMMAN}}/,\'$$'\n/g' | sed 's/{{NL}}/\'$$'\n/g' | sed 's/;/;\'$$'\n/g' | sed 's/^ *//g' >> $@
	@echo "const rez_dir_ent rez_root_dir_ents[] = {" >> $@
	@echo "$(ms.root_initializer)" | sed 's/; /;/g' | sed 's/{{COMMA}}/,/g' | sed 's/{{COMMAN}}/,\'$$'\n/g' | sed 's/;/;\'$$'\n/g' >> $@
	@echo "};" >> $@
	@echo "" >> $@
	@echo "const rez_dir rez_root_dir = { $(words $(ms.root_initializer)), &rez_root_dir_ents[0] };" >> $@
	@echo "" >> $@
	@echo "const rez_dir_ent rez_root_dir_ent = { \"\", (const rez_file_ent*)&rez_root_dir, true };" >> $@
	@echo "" >> $@
	@echo "}" >> $@
	
#
# add all of the resource C++, plus this resource_list.cpp file to the compile list
#
SOURCES+=$(ms.rez_files) $(ms.INTERMEDIATE_DIR)/auto_gen/resource_list.cpp

#################

ms.display_parent_dir=$(if $($(1)_DST_DIR),/resources$($(1)_DST_DIR),/resources)

$(foreach rez,$(sort $(RESOURCES)),$(eval ms.display_rez_fmt_string+=%-40s%s\n))
$(foreach rez,$(sort $(RESOURCES)),$(eval ms.display_rez_data_string+=$(call ms.display_parent_dir,$(rez))/$(notdir $(rez)) $(rez)))

ms.space:=
ms.space+=
ms.display_rez_fmt_string:= $(subst $(ms.space),,$(ms.display_rez_fmt_string))

#
# the 'display_rez' target lets you see what resources are being built into
# the project, where they will be available in the file system at run time
# and where the source files come from
#
.PHONY: display_rez
display_rez:
	@echo ""
	@echo "List of resource files that will be available to open/fopen"
	@echo "at runtime, and where their contents come from, relative to"
	@echo "the current directory:"
	@echo ""
	@printf "%-40s%s\n" "Resource File:" "Original File:"
	@echo "==============                          =============="
	@printf "$(ms.display_rez_fmt_string)" $(ms.display_rez_data_string)
	@echo ""

#
# the case of an empty RESOURCE list, just add the display_rez target
#
else

.PHONY: display_rez
display_rez:
	@echo ""
	@echo "This project contains no resource files in its RESOURCES variable."
	@echo "No resource data will be built into it"
	@echo ""

endif

###############################################################################################
#
#   End of Resource Handling
#
###############################################################################################



ifneq (0,$(ms.DO_NEXE))
 ms.nacl_ext=nexe
else
 ms.nacl_ext=pexe
endif

#
#	$1 the name of the web application you will be building
#
# The list of "interesting" files that this will produce.  This set of files
# will frequently be the ones you will want to move to some kind of "deploy"
# directory as part of building an entire web page that uses these components.
#
ms.TARGET_LIST=\
$(ms.OUT_DIR)/$(CONFIG)/$(1).$(ms.nacl_ext)\
$(ms.OUT_DIR)/$(CONFIG)/$(1).nmf\
$(ms.OUT_DIR)/$(CONFIG)/$(1).js\
$(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem

%.nmf: $(basename $(notdir %)).$(ms.nacl_ext)
	$(call ms.CALL_TOOL,python,$(ms.this_make_dir)nacl_sdk_root/tools/create_nmf.py -o $@ $^ -s $(@D),$@)
