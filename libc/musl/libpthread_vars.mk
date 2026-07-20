CFLAGS         += -include $(CONTRIB_DIR)/src/internal/libc.h
CXXFLAGS       += -include $(CONTRIB_DIR)/src/internal/libc.h
# Only define for C/C++, not for assembly: on MIPS the trampoline assembly uses
# the '.hidden' directive, which would be mangled by a 'hidden' preprocessor
# macro (DEFINES flows into CPPFLAGS and hence into the .S compile).
CFLAGS         += -Dhidden='__attribute__((__visibility__("hidden")))'
CXXFLAGS       += -Dhidden='__attribute__((__visibility__("hidden")))'
CXXFLAGS       += -fno-exceptions
PRIVATE_INCDIR += $(CONTRIB_DIR)/arch/$(LIBC_ARCH)

vpath musl-pthread-impl.c $(LIBCSRC_DIR)/libc/

SRC_C_libc_pthread += musl-pthread-impl.c
