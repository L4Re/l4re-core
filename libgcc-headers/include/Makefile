PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../../..

COMPILER_RT =

include $(L4DIR)/mk/lib.mk

ifneq ($(SYSTEM),)
ifneq ($(CONFIG_COMPILER_RT_USE_LIBGCC),)

SYS_HDR_LOC = $(OBJ_BASE)/include/libgcc-headers

PC_CFLAGS = -isystem $(SYS_HDR_LOC)

LIBGCC_PKG_DIR  = $(PKGDIR)/../libgcc
include $(LIBGCC_PKG_DIR)/contrib.inc

all:: $(SYS_HDR_LOC)/.links-done

$(SYS_HDR_LOC):
	$(VERBOSE)mkdir -p $@

$(SYS_HDR_LOC)/.links-done: $(GENERAL_D_LOC) | $(SYS_HDR_LOC)
	$(VERBOSE)cd $(@D); \
	for i in $(LIBGCC_CONTRIB_DIR)/gcc/ginclude/*; do $(LN) -sf $$i; done; \
	for i in $(LIBGCC_CONTRIB_DIR)/install-include/*; do $(LN) -sf $$i; done; \
	for i in $(LIBGCC_CONTRIB_DIR)/gcc/config/$(CPU_TYPE)/*; do $(LN) -sf $$i; done; \
	touch $@

endif # CONFIG_COMPILER_RT_USE_LIBGCC
endif # SYSTEM
