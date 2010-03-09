build_arch	= x86_64
header_arch	= $(build_arch)
asm_link	= $(build_arch)
defconfig	= defconfig
flavours	= ec2
build_image	= vmlinuz
kernel_file	= arch/$(build_arch)/boot/vmlinuz
install_file	= vmlinuz
loader		= grub

do_libc_dev_package	= false
do_tools		= false
