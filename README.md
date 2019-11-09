# STM32F1 Bluepill
A simple USB Serial IO example with the STM32F103 on the Bluepill using the GCC ARM toolchain, libopencm3 and STLink v2. It uses an interuppt driven polling of the USB.

Toolchain is installed with:

- sudo apt install build-essential
- sudo apt install gcc-arm-none-eabi
- sudo apt install stlink-tools
- sudo apt-get install openocd gdb-multiarch

The last bullet is not neede to be able to build the example, it is to set up a debug environment with gdb and the make targets for debugging.

Also download and build [![libopencm3](https://github.com/libopencm3/libopencm3)].

## Building and flashing
Go into the example directory. This is a quick and ugly makefile to demonstrate this example so it does not contain any fancy stuff in it so you have to go into the makefile and make sure that LIBDIR and INCDIR points to your compiled libopencm3. After that you can build the example and flash the Bluepill.

  $ cd stm32f1_bluepill
  $ make

Plug in the ST-Link:

  $ make burn
  
Remove the ST-Link and plug in the Bluepill to the PC via the USB port and open up a serial terminal (e.g. minicom) for **/dev/ttyACM0** and you should se a welcome message and a prompt to enter something ending with a carriage return. The example will echo back what is typed in and will echo the line after you hit carriage return.

sudo apt-get install openocd gdb-multiarch
