$(OUTROOT)/dev:
	mkdir -p $(OUTROOT)/dev

OUTDIRS += $(OUTROOT)/dev
OBJS += $(patsubst %,$(OUTROOT)/dev/%.o,$(DEVICES))
