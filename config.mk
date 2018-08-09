PLATFORM ?= demo-renode
# Allowed options: none, dma, axidma
DMA ?= none
# Allowed options: yes, no
INTERRUPTS ?= no

OUT ?= out

COMMONFLAGS = -Os -g3 -Wall
CFLAGS 		= \
    -fexceptions -Wstrict-prototypes -Wold-style-definition \
    -fstack-protector
LDFLAGS 	= -nostartfiles

CC := $(TOOLCHAIN_PREFIX)gcc -std=gnu99
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
LD := $(CC)
