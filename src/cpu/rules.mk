$(OUTROOT)/cpu/$(CPU):
	mkdir -p $(OUTROOT)/cpu/$(CPU)

OUTDIRS += $(OUTROOT)/cpu/$(CPU)
OBJS += $(OUTROOT)/cpu/$(CPU)/crt0.o $(OUTROOT)/cpu/$(CPU)/interrupts.o
