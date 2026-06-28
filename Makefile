AS=nasm

TOOLCHAIN=/home/sky/opt/cross
TOOLCHAIN_BIN= $(TOOLCHAIN)/bin
TOOLCHAIN_PREFIX?=i686-elf

export CC=$(TOOLCHAIN_BIN)/$(TOOLCHAIN_PREFIX)-gcc
export LD=$(TOOLCHAIN_BIN)/$(TOOLCHAIN_PREFIX)-ld
CFLAGS=-Wall -Wextra -Iinclude -nolibc -nostdlib -ffreestanding -fpack-struct -m32
LDFLAGS=-T linker.ld --oformat binary -Map=layout.map

OBJDIR=obj
C_SRCS= \
    krnl/kernel.c \
    krnl/shell.c \
    krnl/io.c \
    krnl/string.c \
    krnl/sys/lba.c \
    krnl/fat.c \
    krnl/malloc.c \
    krnl/sys/ps2.c \
    krnl/sys/io.c \
    krnl/sys/serial.c \
    krnl/sys/keymap.c

ASM_SRCS= \
    krnl/link.asm \
    krnl/sys/pic.asm \
    krnl/sys/ports.asm

C_OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(C_SRCS))
ASM_OBJS=$(patsubst %.asm,$(OBJDIR)/%.o,$(ASM_SRCS))
OBJS=$(ASM_OBJS) $(C_OBJS)


all: os.bin

os.bin: obj/boot obj/kernel.bin part.bin
	@echo "OUT   $@"
	@dd if=/dev/zero      of=os.bin bs=512 count=2048
	@dd if=obj/boot       of=os.bin bs=512           conv=notrunc
	@dd if=obj/kernel.bin of=os.bin bs=512 seek=1    conv=notrunc
	@dd if=part.bin       of=os.bin bs=512 seek=2048 conv=notrunc

obj/kernel.bin: $(OBJS)
	@echo "LD    $@"
	$(LD) $(LDFLAGS) -o $@ $^


$(OBJDIR)/boot: boot/boot.asm
	@mkdir -p $(dir $@)
	@echo "BOOT  $<"
	$(AS) -f bin $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "AS    $<"
	$(AS) -f elf32 $< -o $@


debug: os.bin
	qemu-system-i386 -m 512M -hda $< -s -S

run: os.bin
	qemu-system-i386 -m 512M -drive format=raw,file=$< -serial stdio

clean:
	rm -fr obj os.bin layout.map