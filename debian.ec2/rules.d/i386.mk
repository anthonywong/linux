build_arch	= x86
header_arch	= x86
asm_link	= x86
defconfig	= defconfig
flavours        = ec2
build_image	= vmlinuz
kernel_file	= arch/$(build_arch)/boot/vmlinuz
install_file	= vmlinuz
loader		= grub

do_libc_dev_package	= false
do_tools		= false
