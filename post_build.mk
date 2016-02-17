
ms.this_make_dir:=$(dir $(lastword $(MAKEFILE_LIST)))

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
# a newline - also useful in string processing
#
define ms.newline


endef

ifneq (0,$(V))
 ms.CALL_TOOL=$(1) $(2)
else
 ms.CALL_TOOL=@printf "%-17s %s\n" $(notdir $(1)) $(3) && $(1) $(2)
endif

#
# Strategy for only calling mkdir on output directories once.
# We put a file named "dir.stamp" in each of those directories
# and then put an "order only" prerequesite to this file in the
# list of each file to compile.
#
%dir.stamp :
	$(ms.mkdir) -p $(@D)
	$(ms.touch) $@

ms.do_sed = sed "s|\"$1\"|\"$2/$1.`cat $3`\"|g" | sed "s|'$1'|'$2/$1.`cat $3`'|g"

ms.prep_upload_files:=\
$(foreach file,$(UPLOAD_FILES),$(PREP_DIR)/$(notdir $(file)).gz)\
$(foreach file,$(UPLOAD_FILES),$(PREP_DIR)/$(notdir $(file)).sha1)

ms.call_finalize_build:=\
  --config_file=$(realpath $(POST_COMPONENTS_DESC))\
  --prep_dir=$(PREP_DIR)\
  --build_files="$(UPLOAD_FILES)"

ifneq (,$(BUILD_LABEL))
ms.call_finalize_build+=--build_label=$(BUILD_LABEL)
endif

ms.make_frag=$(subst \#,$(ms.newline),$(shell export NODE_PATH=$(NODE_PATH)\
  && node $(ms.this_make_dir)finalize_build.js\
     $(ms.call_finalize_build) | tr '\n' '\#'))
$(eval $(ms.make_frag))

#
# the command string to call the post_build tool,
# passing it lots of information about what we are
# posting and our current world
#
ms.call_upload:=\
export NODE_PATH=$(NODE_PATH) &&\
  node $(ms.this_make_dir)post_build.js\
  --config_file=$(realpath $(POST_COMPONENTS_DESC))\
  --prep_dir=$(PREP_DIR)\
  --build_files="$(UPLOAD_FILES)"\
  --top_files="$(ms.top_components)"\
  --git_user=`git config user.email`\
  --git_head=`git rev-parse HEAD`\
  --git_status="`git status -s`"\
  --git_origin_url=`git config --get remote.origin.url`\
  --git_branch=`git rev-parse --abbrev-ref HEAD`\
  --build_os="`uname -a`"\
  --verbose=$(V)

ifneq (,$(shell which emcc))
ms.call_upload+=--emcc_ver="`emcc --version`"
endif

ifneq (,$(NACL_SDK_ROOT))
ms.getos := python $(NACL_SDK_ROOT)/tools/getos.py
ms.osname := $(shell $(ms.getos))
ms.tc_path := $(realpath $(NACL_SDK_ROOT)/toolchain)
ms.pnacl_tc_bin := $(ms.tc_path)/$(ms.osname)_pnacl/bin
ms.pnacl_cc := $(ms.pnacl_tc_bin)/pnacl-clang
ms.call_upload+=--nacl_ver="`$(ms.pnacl_cc) --version`"
endif

ifneq (,$(BUILD_LABEL))
ms.call_upload+=--build_label=$(BUILD_LABEL)
endif

.PHONY: post_build
post_build post_debug_build: $(ms.prep_upload_files)
	@$(ms.call_upload)

