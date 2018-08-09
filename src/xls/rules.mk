$(OUTROOT)/xls:
	mkdir -p $(OUTROOT)/xls
OUTDIRS += $(OUTROOT)/xls

XLS_SRCS = \
	xls_dma.c

OBJS += $(patsubst %.c,$(OUTROOT)/xls/%.o,$(XLS_SRCS))
