# STM32F1 Bluepill
A simple USB Serial IO example with the STM32F103 on the Bluepill using the GCC ARM toolchain, libopencm3 and STLink v2. It uses an interuppt driven polling of the USB.

Toolchain is installed with:

- sudo apt install build-essential
- sudo apt install gcc-arm-none-eabi
- sudo apt install stlink-tools

Also download and build [![libopencm3](https://github.com/libopencm3/libopencm3)].

## Building and flashing
Go into the example directory:

  $ cd stm32f1_bluepill
  $ make

Plug in the ST-Link:

  $ make burn
  
Remove the ST-Link and plug in the Bluepill to the PC via the USB port and open up a serial terminal (e.g. minicom) for **/dev/ttyACM0** and you should se a welcome message and a prompt to enter something ending with a carriage return. The example will echo back what is typed in and will echo the line after you hit carriage return.

