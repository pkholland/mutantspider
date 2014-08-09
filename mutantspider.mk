#
#	WebRules core makefile.  See README.makefile for basic documentation
#
#	Some naming conventions:
#		1) variables defined by MutantSpider will be in the ms "namespace".  That is, they will be of the form
#			ms.VariableName
#		2) exceptions to #1 include:
#			a) variables that are reasonable to type on the command line, for example "V" and "CONFIG"
#			b) the collection of compiler and linker flags (CFLAGS, CFLAGS_release, etc...)
#		3) variable names in ms of the form ms.ALL_CAPS_AND_UNDERSCORES are expected to be "public"
#			in the sense that code outside of mutantspider.mk can reasonably expect to use and manipulate those
#		4) variable names in ms of the form ms.lower_case are expected to be "private" and should not
#			be used outside of mutantspider.mk
#
#	TODO list:
#		1) provide explicit bindings of source file suffix -> compiler, and then use $(filter ...) to figure
#			out which files need to be expanded with which build rules
#		2) change configuration validation (installed compilers, etc...) so that it is only triggered if
#			you attempt to build something with that compiler.  Once #1 is done, if *.ts -> the typescript compiler
#			then it should only check to make sure you have the typescript compiler installed if your SOURCES list
#			contains *.ts files.  This can be done similar to the way the dir.stamp files are handled
#		3) provide some kind of support for specifying which C compilers you want to use.  For example, if you
#			only want to generate asm.js (and not pnacl) there should be some way to specify this.
#		4) DO_STRIP is currently only running if the target is exactly "release".  Fix this somehow.  It's also
#			violating the naming conventions


#
# the directory that this makefile (mutantspider.mk) is in -- relative to the orignal
# directory that make was invoked in $(CURDIR)
#
ms.this_make_dir:=$(dir $(lastword $(MAKEFILE_LIST)))

#
# Make sure there is a nacl_sdk_root symlink directory
#
ifeq (,$(wildcard $(ms.this_make_dir)nacl_sdk_root))
 $(info *********************************)
 $(info 'nacl_sdk_root' directory missing)
 $(info Please create a symbolic link named 'nacl_sdk_root' in $(realpath $(ms.this_make_dir)),)
 $(info pointing to the NaCl/Pepper sdk directory you want to use)
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
 $(info ("which emcc" failed to find it))
 $(error )
endif

#
# Root directory into which temp .o files will be placed
#
ifeq (,$(ms.INTERMEDIATE_DIR))
 ms.INTERMEDIATE_DIR:=obj
 $(info ms.INTERMEDIATE_DIR not set, so defaulting to $(CURDIR)/$(ms.INTERMEDIATE_DIR))
 $(info To remove this warning set ms.INTERMEDIATE_DIR = something prior to including mutantspider.mk)
endif

#
# Root directory into which final executables will be placed
#
ifeq (,$(ms.OUT_DIR))
 ms.OUT_DIR:=out
 $(info ms.OUT_DIR not set, so defaulting to $(CURDIR)/$(ms.OUT_DIR))
 $(info To remove this warning set ms.OUT_DIR = something prior to including mutantspider.mk)
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

ms.em_cc := emcc
ms.em_cxx := emcc
ms.em_link := emcc

#
# Strategy for only calling mkdir on output directories once.
# We put a file named "dir.stamp" in each of those directories
# and then put a "order only" prerequesite to this file in the
# list of each file to compile.
#
%dir.stamp :
	$(ms.mkdir) -p $(@D)
	$(ms.touch) $@

#
# Convert a source path to an object file path.
#
# $1 = Source Name
# $2 = Arch suffix
#
ms.src_to_obj=$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(basename $(subst ..,__,$(1)))$(2).o

#
# Convert a source path to a dependency file path.
#
# $1 = Source Name
# $2 = Arch suffix
#
ms.src_to_dep=$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(basename $(subst ..,__,$(1)))$(2).d

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
 ms.call_tool=$(1) $(2)
else
 ms.call_tool=@printf "%-17s %s\n" $(notdir $(1)) $(3) && $(1) $(2)
endif

#
# call the symbol stripping tool either with full echoing (verbose is on)
# or silently (verbose is off).  This function strips to a ".strip" file
# and then replaces the original with the .strip file.
#
# $1 = (fully path-qualified, if necessary) name of strip tool
# $2 = name of file to be stripped
#
ifeq (release,$(CONFIG))
 ms.do_strip = $(call ms.call_tool,$(1),-o $(2).strip $(2),$(2)) && rm $(2) && mv $(2).strip $(2)
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
# TODO: check on closure 1
#
CFLAGS_emcc_release+=-s FORCE_ALIGNED_MEMORY=1 -O2 -s DOUBLE_MODE=0 -s DISABLE_EXCEPTION_CATCHING=0 --llvm-lto 1
LDFLAGS_emcc_release+=-s FORCE_ALIGNED_MEMORY=1 -O2 -s DOUBLE_MODE=0 -s DISABLE_EXCEPTION_CATCHING=0 --llvm-lto 1 --closure 0

CFLAGS_emcc_debug+=-s ASSERTIONS=1
LDFLAGS_emcc_debug+=-s ASSERTIONS=1

CFLAGS_emcc+=--memory-init-file 1
LDFLAGS_emcc+=--memory-init-file 1



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
# helper target to see what the compile options are set to
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
	$(call ms.call_tool,$(ms.pnacl_cc),-o $$@ -c $$< -MD -MF $(call ms.src_to_dep,$(1),_pnacl) $(2) $(CFLAGS) $(CFLAGS_pnacl) $(CFLAGS_$(CONFIG)) $(CFLAGS_pnacl_$(CONFIG)) $(CFLAGS_pnacl_$(1)),$$@)

$(call ms.src_to_obj,$(1),_js): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_emcc.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.call_tool,$(ms.em_cc),-o $$@ $$< -MD -MF $(call ms.src_to_dep,$(1),_js)d $(2) $(CFLAGS) $(CFLAGS_$(CONFIG)) $(CFLAGS_emcc) $(CFLAGS_emcc_$(CONFIG)) $(CFLAGS_emcc_$(1)),$$@)
	$(ms.sed) '1 c\'$$$$'\n''$(call ms.src_to_obj,$(1),_js): \\'$$$$'\n' $(call ms.src_to_dep,$(1),_js)d > $(call ms.src_to_dep,$(1),_js)
	@rm $(call ms.src_to_dep,$(1),_js)d

endef

define ms.cxx_compile_rule
$(call ms.src_to_obj,$(1),_pnacl): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_pnacl.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.call_tool,$(ms.pnacl_cxx),-o $$@ -c $$< -MD -MF $(call ms.src_to_dep,$(1),_pnacl) $(2) -std=gnu++11 $(CFLAGS) $(CFLAGS_pnacl) $(CFLAGS_$(CONFIG)) $(CFLAGS_pnacl_$(CONFIG)) $(CFLAGS_pnacl_$(1)),$$@)

$(call ms.src_to_obj,$(1),_js): $(1) $(ms.INTERMEDIATE_DIR)/$(CONFIG)/compiler_emcc.opts | $(dir $(call ms.src_to_obj,$(1)))dir.stamp
	$(call ms.call_tool,$(ms.em_cxx),-o $$@ $$< -MD -MF $(call ms.src_to_dep,$(1),_js)d $(2) -std=c++0x $(CFLAGS) $(CFLAGS_$(CONFIG)) $(CFLAGS_emcc) $(CFLAGS_emcc_$(CONFIG)) $(CFLAGS_emcc_$(1)),$$@)
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
# ms.DO_NEXE can be set to force this to happen
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
	$(call ms.call_tool,$(ms.pnacl_ar), -cr $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a $$(filter-out %.opts,$$^),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a)
	$(ms.mkdir) -p $$(@D)
	$(call ms.call_tool,$(ms.pnacl_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_pnacl) $(LDFLAGS_pnacl_$(CONFIG)) -L$(ms.lib_root)/$(ms.NACL_LIB_PATH) -L$(ms.INTERMEDIATE_DIR)/$(CONFIG) -o $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc -lppapi -lppapi_cpp -l$(1) -lppapi -lppapi_cpp $(foreach lib,$(3),-l$(lib)),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc)
	$(call ms.call_tool,$(ms.pnacl_translate), --allow-llvm-bitcode-input -arch i686 $(ms.INTERMEDIATE_DIR)/$(CONFIG)/$(1).bc -o $$@, $$@)

endef
else
define ms.NACL_LINKER_RULE
$(ms.OUT_DIR)/$(CONFIG)/$(1).pexe: $(ms.INTERMEDIATE_DIR)/$(CONFIG)/linker_pnacl.opts $(foreach src,$(filter-out $(pnacl_EXCLUDE),$(2)),$(call ms.src_to_obj,$(src),_pnacl))
	@rm -f $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a
	$(call ms.call_tool,$(ms.pnacl_ar), -cr $(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a $$(filter-out %.opts,$$^),$(ms.INTERMEDIATE_DIR)/$(CONFIG)/lib$(1).a)
	$(ms.mkdir) -p $$(@D)
	$(call ms.call_tool,$(ms.pnacl_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_pnacl) $(LDFLAGS_pnacl_$(CONFIG)) -L$(ms.lib_root)/$(ms.NACL_LIB_PATH) -L$(ms.INTERMEDIATE_DIR)/$(CONFIG) -o $$@ -lppapi -lppapi_cpp -l$(1) -lppapi -lppapi_cpp $(foreach lib,$(3),-l$(lib)),$(ms.OUT_DIR)/$(CONFIG)/$(1).pexe)
	$(call ms.do_strip,$(ms.pnacl_strip),$$@)
	$(call ms.call_tool,$(ms.pnacl_finalize),$$@,$$@)

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
	$(call ms.call_tool,$(ms.em_link),$(LDFLAGS) $(LDFLAGS_$(CONFIG)) $(LDFLAGS_emcc) $(LDFLAGS_emcc_$(CONFIG)) -o $$@ $$(filter-out %.opts,$$^),$(ms.OUT_DIR)/$(CONFIG)/$(1).js)

#
# bug in gnumake???
# the rule to build the .js prerequisite also builds the .mem (this target)
# so we shouldn't really need any recipe here, and just an empty recipe does
# cause something like "make <mycomponent>.js.mem" _does_ cause it to build the
# file (by executing the recipe above for producing the .js).  But for some
# reason I can't figure out, later recipies in targets that have this .js.mem
# as a dependency don't run if the full list of targets also includes the .js
# file, and if this recipe is empty.  So we just execute a practically-do-nothing
# "touch" here as the recipe.  This is enough to get the later recipes to run
# ???
#
$(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem: $(ms.OUT_DIR)/$(CONFIG)/$(1).js
	@touch $(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem

endef

ifneq (0,$(ms.DO_NEXE))
 ms.nacl_ext=nexe
else
 ms.nacl_ext=pexe
endif

ms.TARGET_LIST=\
$(ms.OUT_DIR)/$(CONFIG)/$(1).$(ms.nacl_ext)\
$(ms.OUT_DIR)/$(CONFIG)/$(1).nmf\
$(ms.OUT_DIR)/$(CONFIG)/$(1).js\
$(ms.OUT_DIR)/$(CONFIG)/$(1).js.mem

%.nmf: $(basename $(notdir %)).$(ms.nacl_ext)
	$(call ms.call_tool,python,$(ms.this_make_dir)nacl_sdk_root/tools/create_nmf.py -o $@ $^ -s $(@D),$@)
