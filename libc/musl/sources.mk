# Directories
DIRS-all := ctype env locale ldso string stdio stdlib internal multibyte errno exit syscall legacy misc setjmp
DIRS-all += mman
DIRS-all += signal prng
DIRS-all += $(if $(BID_VARIANT_FLAG_NOFPU),,math fenv)

DIRS-minimal := $(DIRS-all) syscalls
DIRS-full    := $(DIRS-all) thread unistd malloc temp time dirent regex passwd
DIRS-full    += termios network

DIRS         := $(DIRS-$(LIBC_BUILD_MODE))

# Sub Modules
SUB_MODULES-all     :=

SUB_MODULES-minimal := $(SUB_MODULES-all)
SUB_MODULES-full    := $(SUB_MODULES-all) wchar \
                       $(if $(BID_VARIANT_FLAG_NOFPU),,fp)

SUB_MODULES := $(SUB_MODULES-$(LIBC_BUILD_MODE))

# CRT files
NAME_crt1        = crt1.c
NAME_crt1_shared = Scrt1.c
NAME_crt1_reloc  = rcrt1.c
# Most arches provide crt/<arch>/crti.s and crtn.s in assembly. Some (e.g.
# riscv) have no arch-specific version and use the generic crt/crti.c and
# crt/crtn.c instead.
NAME_crti        = $(if $(wildcard $(CONTRIB_DIR)/crt/$(LIBC_ARCH)/crti.s),crti.s,crti.c)
NAME_crtn        = $(if $(wildcard $(CONTRIB_DIR)/crt/$(LIBC_ARCH)/crtn.s),crtn.s,crtn.c)
