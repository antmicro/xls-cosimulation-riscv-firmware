include config.mk
include src/platform/$(PLATFORM)/config.mk # depends on ^
include src/cpu/$(CPU)/config.mk # depends on ^

default: all

OUTROOT = $(OUT)/$(PLATFORM)
$(OUTROOT):
	mkdir -p $(OUTROOT)

include src/dev/rules.mk
include src/cpu/rules.mk
include src/xls/rules.mk
include src/common/rules.mk

OUTDIRS += $(OUTROOT)

ALL_CFLAGS = \
	$(CPUFLAGS) $(COMMONFLAGS) $(CFLAGS) $(CDEFS) \
	-DPLATFORM_$(shell echo $(PLATFORM) | tr a-z\\- A-Z_) \
	$(patsubst %,-DDEV_%,$(shell echo $(DEVICES) | tr a-z\\- A-Z_)) \
	$(patsubst %,-DCPU_%,$(shell echo $(CPU) | tr a-z\\- A-Z_)) \
	-Isrc
ALL_LDFLAGS = $(CPUFLAGS) $(LDFLAGS)

FW_PREFIX = $(OUTROOT)/fw_$(PLATFORM)

all: $(FW_PREFIX).elf $(FW_PREFIX).dump $(FW_PREFIX).map

ifeq ($(DMA),dma)
  ALL_CFLAGS += -DRLE_DMA
endif
ifeq ($(DMA),axidma)
  ALL_CFLAGS += -DRLE_DMA -DRLE_DMA_AXI
endif

ifeq ($(INTERRUPTS),yes)
  ALL_CFLAGS += -DRLE_DMA_IRQ
endif

$(OUT):
	mkdir -p $(OUT)

%.dump: %.elf
	$(OBJDUMP) -S $< > $@

$(FW_PREFIX).elf $(FW_PREFIX).map: $(OBJS) src/platform/$(PLATFORM)/linker.ld
	$(LD) $(ALL_LDFLAGS) -T src/platform/$(PLATFORM)/linker.ld \
	-Wl,-Map $(FW_PREFIX).map -N -o $@ $(OBJS)

$(OUTROOT)/%.o: src/%.c $(OUTDIRS)
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

$(OUTROOT)/%.o: src/%.S $(OUTDIRS)
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

clean:
	rm -v -r $(OUTROOT)

.PHONY: all clean
