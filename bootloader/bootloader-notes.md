# Hardware debugging / Flashing a bootloader to WhiteFox

## Overview

ARM chips have hardware debugging capabilities (e.g. you can stop the execution of the current firmware, display/modify memory content, go step-by-step through the program, etc...)

This can be used for debugging, but also for flashing new firmware to the board, including the bootloader (which is not possible with the DFU bootloader, because it protects itself). This is very powerful, but potentially dangerous, because you can brick your board (this is not a general abstract warning -- Freescale has built a mechanism into the MCU that does this; it's a security feature that disables any modifications to the firmware). If you use the DFU bootloader, you do not need to worry about any of these things.

Generally speaking, the high-level picture is as follows: you connect a hardware device (*a debugger*) between your PC and the WhiteFox, and run a computer program that, on one side, talks to the *debugger* (and via it, to the MCU), and on the other side with the user (you).

        [WhiteFox] --- [hardware debugger] --- [program] @ [PC]

## Phase 1 (only if you want to flash a bootloader): get the bootloader firmware

### Option 1: download from somewhere

There is one in this directory (`whitefox-bootloader.bin`); it is from one of the actual WhiteFoxes. 

### Option 2: compile yourself

**Warning:** This may not end up working. According to haata, his various boards need various particular git commits.

1. Get an ARM toolchain (i.e. programs like the `arm-none-eabi-gcc` compiler). Either install manually from [here](https://launchpad.net/gcc-arm-embedded/+download), or install packages through the system package manager on linux, or use [homebrew](http://brew.sh/) on Mac OS X (search for the right tap that has it).

2. On top of the toolchain, you'll need `cmake` (and maybe something else, please add here if you come across another dependency).

3. Get the sources - haata's repo on [github](https://github.com/kiibohd/controller) - either clone with `git` or click the 'Download ZIP' button and unpack.

4. Edit the configs to generate the bootloader for the right chip: edit `CMakeLists.txt`, comment out `mk20dx128vlf5` and uncomment `mk20dx256vlh7`.

5. Compile:

        cd Bootloader
        mkdir build
        cd build
        cmake ..
        make

    You should end up with a couple of files named `kiibohd_bootloader.*` - the `bin` is the binary firmware.

## Phase 2: connect a debugger

Hopefully connecting the debugger to the PC is a matter of having the right USB cable. If you have the right [Tag Connect](http://www.tag-connect.com/) cable for the header that's on the WF PCB, then connect it.

If you don't have that debugger-WF cable (it's kinda expensive), you can just use jumper wires. The protocol that we'll be using is SWD, which requires at least three wires: `Ground`, `SWDIO` and `SWCLK`. Possibly also `3.3V` or `5V` if you intend to power WF from the debugger - alternatively plug an USB cable into WF, and only connect the first three (recommended).

All these signals are brought out to pads on the long row of pads. Different naming convention, so `VSS = Ground`, `DIO = SWDIO`, `CLK = SWCLK`, `VDD = 3.3V`, `+5V = +5V`. Do _not_ connect both 3.3V and 5V to power sources at the same time.

Where are these on your debugger depends on the debugger: if you have a ST-Link (there are some very cheap clones on ebay) or something like that, they tend to have _only_ SWD connections, and usually in the order that's on WF PCB. If you have a proper JTAG debugger, the pinouts can be seen [here](http://www.keil.com/support/man/docs/ulink2/ulink2_hw_connectors.htm) - look for 'Serial Wire' mode.

By the way, ST-Link is integrated on ST Discovery boards and ST Nucleo boards. So if you have any of those, you can use them as a programmer. Similarly, a CMSIS-DAP debugger (after firmware update) is integrated on Freescale's Freedom (e.g. FRDM-KL25Z, ...) boards; you can use that one as well (although you'll need some funky 1.27mm pitch connectors).

While ST-Link works great with STM32 MCUs, I **do not recommend** using it with WhiteFox for flashing the bootloader. It is not reliable enough (because it uses a slightly less powerful protocol than SWD for non-STM32 MCUs), and you can brick your WhiteFox. And by brick I mean if you're lucky and get a different debugger you might be able to recover (see below), but you may be unlucky and then the only thing that helps is to solder a new MCU onto the board.

However ST-Links are fine for debugging the WhiteFox.

## Phase 3: flash and/or debug

Once everything's physically connected, it's a matter of communicating with the debugger. There are, as usual, options - e.g. if you have a proper debugger, it probably comes with some software. However, here's an open source way; with instructions geared towards proper systems, i.e. linux or OS X.

### Openocd

The general connection diagram is

        ... [a hardware debugger] ---<wires>--- [openocd @ PC] ---<TCP/IP>--- [debugger program]

In other words, openocd talks to the debugger, and exposes itself as a service over TCP/IP. One can talk to it either directly (`telnet`) or via a more sophisticated interface that also knows about your C sources, etc... (usually `gdb` or something on top of `gdb` (e.g. eclipse or cgdb)).

1. Install [openocd](http://openocd.org/). I recommend system packages on linux or [homebrew](http://brew.sh/), unless they're too old (I've tested things with 0.9.0, so for older versions YMMV).

2. Get a config file (you can make one yourself if you're familiar with openocd, there are no special tricks as far as I can tell): 

  - for jlink from [here](https://github.com/flabbergast/chibios-projects/blob/master/openocd/wf-jlink.cfg) (click 'Raw').
  - for cmsis-dap [here](https://github.com/flabbergast/chibios-projects/blob/master/openocd/wf-cmsis-dap.cfg) (click 'Raw').
  - for stlink [here](https://github.com/flabbergast/chibios-projects/blob/master/openocd/wf-stlink.cfg) (click 'Raw').

3. You can try connecting openocd/debugger to WF by just

        openocd -f wf-jlink.cfg

   It should keep running without spewing out any errors. Communicating with openocd is now via TCP/4444 port with telnet, or TCP/3333 with gdb for debugging.

4. You can flash a binary with this command:

        openocd -f wf-jlink.cfg -c "program wf-boot.bin 0x0 verify reset exit"

   where `wf-boot.bin` is the name of the file containing the firmware (plain binary format! Do **not** flash elfs or hexes - it is possible to irrecoverably brick the chip by flashing a wrong byte at `0x40c`.)

### Debugging

Do 1., 2. and 3. above. Keep openocd running. You can connect gdb to it by (assuming you're in the `whitefox` directory and you compiled the firmware)

        arm-none-eabi-gdb build/ch.elf -ex "target extended-remote :3333"

### Troubleshooting

If you've messed up the flash security byte (`0x40c` of the flash), when you run openocd you'll probably see error messages like

        Polling target k40.cpu failed, trying to reexamine
        Info : SWD IDCODE 0x2ba01477
        Info : SWD IDCODE 0x2ba01477
        Error: Failed to read memory at 0xe000ed00
        Examination failed, GDB will be halted. Polling again in 6300ms

and

        Warn : *********** ATTENTION! ATTENTION! ATTENTION! ATTENTION! **********
        Warn : ****                                                          ****
        Warn : **** Your Kinetis MCU is in secured state, which means that,  ****
        Warn : **** with exception for very basic communication, JTAG/SWD    ****
        Warn : **** interface will NOT work. In order to restore its         ****
        Warn : **** functionality please issue 'kinetis mdm mass_erase'      ****
        Warn : **** command, power cycle the MCU and restart OpenOCD.        ****
        Warn : ****                                                          ****
        Warn : *********** ATTENTION! ATTENTION! ATTENTION! ATTENTION! **********

To fix this, keep openocd running, get a WF bootloader binary (`wf-boot.bin`) and run

        telnet localhost 4444

This will connect to the openocd process, and you are able to issue commands to openocd. So now:

        kinetis mdm mass_erase

and then

        program wf-boot.bin 0x0

Finally `exit` will exit the telnet/openocd session. After re-plugging or resetting WF, it should enumerate in the DFU mode, and the onboard LED should be on.

Note: you might need to fiddle a bit with the hardware to get the `mass_erase` command to execute successfully (e.g. hold the reset button during or just before issuing the command). You should see

        Info : MDM: Chip is unsecured. Continuing.

from openocd if the `mass_erase` was successful.
