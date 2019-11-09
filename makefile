CC = arm-none-eabi-gcc
OC = arm-none-eabi-objcopy
BURN = st-flash
OPENOCD = openocd
GDB = gdb-multiarch

FLAGSC	= -Os -std=c99 -ggdb3 -mthumb -mcpu=cortex-m3 -msoft-float -mfix-cortex-m3-ldrd -Wextra -Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -fno-common -ffunction-sections -fdata-sections  -MD -Wall -Wundef -DSTM32F1

FLAGSL		= --static -nostartfiles -Tbluepill.ld -mthumb -mcpu=cortex-m3 -msoft-float -mfix-cortex-m3-ldrd -ggdb3 -Wl,--cref -Wl,--gc-sections -lopencm3_stm32f1 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

INCDIR		= ../libopencm3/include
LIBDIR		= ../libopencm3/lib

pill.bin: main.o sio.o usb.o delay.o
	$(CC) main.o sio.o usb.o delay.o -L${LIBDIR} -Wl,-Map=pill.map ${FLAGSL} -o pill.elf
	$(OC) -O binary pill.elf pill.bin

%.o: %.c
	$(CC) ${FLAGSC} -I${INCDIR} -o $(*).o -c $(*).c

burn:
	$(BURN) write pill.bin 0x08000000

debug:
	$(OPENOCD) -f bluepill.cfg

gdb:
	$(GDB) --eval-command="target remote localhost:3333" pill.elf

clean:
	rm *.o *.d *.map *.bin *.elf

-include *.d
