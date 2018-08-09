$(OUTROOT)/common:
	mkdir -p $(OUTROOT)/common
OUTDIRS += $(OUTROOT)/common

COMMON_SRCS = \
	syscalls.c \
	main.c

OBJS += $(patsubst %.c,$(OUTROOT)/common/%.o,$(COMMON_SRCS))
