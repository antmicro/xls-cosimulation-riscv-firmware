# XLS RLE encoder IR co-simulation demo/reference firmware

Copyright (C) 2023-2024 Antmicro

This repository contains sources and necessary platform description files
for building and simulating a system that consists of a VexRiscV CPU
and an RLE encoder simulated with XLS framework from IR sources.

The demo showcases different ways XLS channels can be exposed to the software
and provides adequate abstractions for handling communication with them:

* XLS streams (polling)
* XLS DMA (polling)
* XLS DMA (interrupts)
* XLS DMA with AXI-like bus (polling)
* XLS DMA with AXI-like bus (interrupts)

The firmware exposes an interactive prompt via UART. In this prompt the user can
type in a string. Once they hit return, that string will be sent to the RLE
encoder and then the encoded result will be printed as a series of tuples, where
the first element is the character from the input string and the second is the
number of repetitions. The last tuple will be marked with `(last)` text, unless
the program is compiled to use DMA with AXI-like bus (the `last` bit is removed
from the payload because it's handled by the bus).

Note that this demo is supposed to showcase all ways in which software can
communicate with an XLS device. It is supposed to serve as a reference for
handling various scenarios, but it is NOT supposed to be an example of adequate
communication choices for a given design. The non-AXI DMA examples rely on
timeouts because the received output length can't be computed beforehand.
In such cases, an AXI-like DMA, or a polled stream would be recommended.

# Building

To build this, please use RiscV GCC toolchain with Newlib libc.
This firmware was developed and tested with a toolchain built with
[crosstool-ng](https://github.com/crosstool-ng/crosstool-ng):
* **GCC**: 13.2.0 (riscv64)
* **newlib**: 4.3.0.20230120.

If your compiler binary is called *riscv-something-gcc*, run the following
to build:
```
TOOLCHAIN_PREFIX=path/to/toolchain/bin/riscv-something-   make
```

The final firmware output is in *out/demo-renode* directory:
```
out/demo-renode/fw_demo-renode.elf
```

You can configure the firmware using make variables, or by modifying config.mk:

* default config - use XLS Streams (polling)
* `DMA=dma` - use XLS DMA
* `DMA=axidma` - Use XLS AXI-like DMA
* `INTERRUPTS=yes` - Use interrupts (available only if DMA!=no.) instead of
  polling

# Obtaining the library

Build `//xls/simulation/renode:renode_xls_peripheral_plugin` from XLS repository and
copy it into *lib/* directory (or place a symlink):

```
% cp -vf /path/to/librenode_xls_peripheral_plugin.so ./lib/
```

# Running in a simulator

## Renode

### For the stream-based demo:

Set the `path_to_ir_design` field in `rle_enc_sm.textproto` file to point to the
RLE IR design.

Then run:
```
renode --disable-xwt --console ./vexriscv_rle.resc
```

### For the DMA-based demos:

Set the `path_to_ir_design` field in `rle_enc_sm_dma.textproto` file to point to the
RLE IR design.

Then run:
```
renode --disable-xwt --console ./vexriscv_rle_dma.resc
```

### For the AXI-DMA-based demos:

Set the `path_to_ir_design` field in `rle_enc_sm_axidma.textproto` file to point to the
RLE IR design.

```
renode --disable-xwt --console ./vexriscv_rle_axidma.resc
```

--------------

If the connection is successful, you should see something along those lines in Renode
output:

```
21:33:05.5273 [INFO] xls0: I1009 21:33:05.526413   94451 xlsperipheral.cc:280] Creating renode::XlsPeripheral. Config: /path/to/rle_enc_sm_axidma.textproto
21:33:05.5308 [INFO] xls0: I1009 21:33:05.530721   94451 xlsperipheral.cc:112] Setting up simulation of the following design: "path/to/rle_enc.ir"
21:33:05.5533 [INFO] xls0: I1009 21:33:05.553158   94451 xlsperipheral.cc:244] Created channel "rle_enc__input_r"
21:33:05.5533 [INFO] xls0: I1009 21:33:05.553278   94451 xlsperipheral.cc:244] Created channel "rle_enc__output_s"
21:33:05.5533 [INFO] xls0: I1009 21:33:05.553343   94451 xlsperipheral.cc:536] Peripheral has been reset
```

In order to interact with the firmware, you'll need to connect to `sysbus.uart0`:
https://renode.readthedocs.io/en/latest/host-integration/uart.html

Once you are connected, type `start` or `s` into Renode terminal to start
emulation. You should be greeted with an interactive prompt on the `sysbus.uart0`
terminal:

```
check_init: Seems that we started cleanly.
RISC-V CSRs: MVENDORID=00000000, MARCHID=00000000, MIMPID=00000000, MHARTID=00000000, MSTATUS=00000000, MISA=40041105, MIE=00000000
Enter RLE input:
```

## Gem5 (Pending)

The Renode version of the firmware is contains VexRiscV-specific code as that's the CPU
simulated in Renode. Gem5 however does not support that CPU. It support SiFive U54-MC,
which is another RISC-V CPU. It's interrupt controller works differently that the one
found on VexRiscV, hence the firmware has to be adapted to it.

The mainline branch of Gem5 does not expose any API for co-simulation, so currently the
Gem5 integration requires our fork of Gem5, but that's yet to be published.
This README will be updated with information required for running the demo under Gem5
once the fork becomes publicly available.
