
.PHONY: all clean debug release run_server run_debug_server help
all:

#
# set CONFIG for the convience targets 'debug' and 'run_debug_server'
# we don't need to worry about either of the release targets because
# the default CONFIG is release
#
ifeq (debug,$(MAKECMDGOALS))
CONFIG=debug
endif
ifeq (run_debug_server,$(MAKECMDGOALS))
CONFIG=debug
endif

#
# simple 'help' target that shows you some interesting targets you can use
#
help:
	@cat make.help

#
# these two targets build 'all'
#
debug: all
release: all

#
# the list of C/C++ files to compile
#
SOURCES=\
$(wildcard *.cpp)\
$(wildcard *.c)

#
# where and what we are building
#
ms.INTERMEDIATE_DIR=ms_tmp/obj
ms.OUT_DIR=ms_tmp/out
DEPLOY_DIR=deploy
BUILD_NAME=hello_spider

#
# defines the functions we use below
#
include ../../src/mutantspider.mk


#
# create build/dependency rules for all sources and all compilers
#
$(foreach src,$(SOURCES),$(eval $(call ms.COMPILE_RULE,$(src))))

#
# create build/dependency rules for all of the nacl targets
#
$(eval $(call ms.NACL_LINKER_RULE,$(BUILD_NAME),$(SOURCES)))

#
# create a build/dependency rule for the asm.js target
#
$(eval $(call ms.EM_LINKER_RULE,$(BUILD_NAME),$(SOURCES)))

clean:
	rm -rf $(DEPLOY_DIR) ms_tmp

#############################################################

#
# the files that mutantspider.mk will build
#
MS_TARGETS:=$(call ms.TARGET_LIST,$(BUILD_NAME))

#
# separate them by extension so we can treat
# them differently in the deployment
#
NACL_TARGETS:=$(filter %.pexe %.nmf,$(MS_TARGETS))
JS_TARGETS:=$(filter %.js %.js.mem,$(MS_TARGETS))

#
# make sure web_rules isn't defining some target we don't know how to handle
#
UNKNOWN_TARGETS:=$(filter-out $(NACL_TARGETS),$(filter-out $(JS_TARGETS),$(MS_TARGETS)))
ifneq (,$(UNKNOWN_TARGETS))
 $(info Don't know how to deploy $(UNKNOWN_TARGETS) (produced by mutantspider.mk))
 $(error )
endif

#
# list of files which constitute a deployment
#
DEPLOY_FILES=\
$(DEPLOY_DIR)/$(CONFIG)/index.html\
$(patsubst $(ms.OUT_DIR)/$(CONFIG)%,$(DEPLOY_DIR)/$(CONFIG)%,$(MS_TARGETS))\
$(DEPLOY_DIR)/$(CONFIG)/hello_spider_boots.js\
$(DEPLOY_DIR)/$(CONFIG)/mutantspider.js

#
# make sure that 'all' builds all of the deploy files
#
all: $(DEPLOY_FILES)

#
# template for build rule to copy/link files from some src location into some deploy/$(CONFIG) location
#
#  $1 = the path, relative to the current directory, _WITHOUT_ a trailing '/', that contains the source file
#  $2 = the name of the file itself (with no path prefix)
#
define copy_template
$(DEPLOY_DIR)/$(CONFIG)/$(2): $(1)/$(2)
	@mkdir -p $$(@D)
	$(call ms.CALL_TOOL,ln,-f $$< $$@,$$@)

endef

#
# instantiate an instance of copy_template for each file in $(MS_TARGETS)
#
$(foreach cp_file,$(MS_TARGETS),$(eval $(call copy_template,$(ms.OUT_DIR)/$(CONFIG),$(notdir $(cp_file)))))

#
# instantiate the copy_template for the three files we just copy to the deployment directory
#
$(eval $(call copy_template,.,index.html))
$(eval $(call copy_template,../../src,mutantspider.js))
$(eval $(call copy_template,.,hello_spider_boots.js))

########################################

#
# both of the "run server" targets build all and then
# run the nacl-provided http deamon tool on the resulting
# deployment directory
#
run_server run_debug_server: all
	@cd $(DEPLOY_DIR)/$(CONFIG) && ../../../../src/nacl_sdk_root/tools/httpd.py --no-dir-check
	


