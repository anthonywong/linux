# We don't want make removing intermediary stamps
.SECONDARY :

# Prepare the out-of-tree build directory

prepare-%: $(stampdir)/stamp-prepare-%
	@# Empty for make to be happy
$(stampdir)/stamp-prepare-%: target_flavour = $*
$(stampdir)/stamp-prepare-%: $(confdir)/config $(confdir)/config.%
	@echo "Preparing $*..."
	install -d $(builddir)/build-$*
	touch $(builddir)/build-$*/ubuntu-build
	cat $^ | sed -e 's/.*CONFIG_VERSION_SIGNATURE.*/CONFIG_VERSION_SIGNATURE="Ubuntu $(release)-$(revision)-$*"/' > $(builddir)/build-$*/.config
	find $(builddir)/build-$* -name "*.ko" | xargs rm -f
	$(kmake) O=$(builddir)/build-$* silentoldconfig prepare scripts
	touch $@


# Do the actual build, including image and modules
build-%: $(stampdir)/stamp-build-%
	@# Empty for make to be happy
$(stampdir)/stamp-build-%: target_flavour = $*
$(stampdir)/stamp-build-%: $(stampdir)/stamp-prepare-%
	@echo "Building $*..."
	$(kmake) O=$(builddir)/build-$* $(conc_level) $(build_image)
	$(kmake) O=$(builddir)/build-$* $(conc_level) modules
	@touch $@

# Install the finished build
install-%: pkgdir = $(CURDIR)/debian/linux-image-$(release)$(debnum)-$*
install-%: dbgpkgdir = $(CURDIR)/debian/linux-image-debug-$(release)$(debnum)-$*
install-%: basepkg = linux-headers-$(release)$(debnum)
install-%: hdrdir = $(CURDIR)/debian/$(basepkg)-$*/usr/src/$(basepkg)-$*
install-%: target_flavour = $*
install-%: $(stampdir)/stamp-build-% checks-%
	dh_testdir
	dh_testroot
	dh_clean -k -plinux-image-$(release)$(debnum)-$*
	dh_clean -k -plinux-headers-$(release)$(debnum)-$*
	dh_clean -k -plinux-image-debug-$(release)$(debnum)-$*

	# The main image
ifeq ($(compress_file),)
	install -m644 -D $(builddir)/build-$*/$(kernel_file) \
		$(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$*
else
	install -d $(pkgdir)/boot/
	gzip -c9v $(builddir)/build-$*/$(kernel_file) > \
		$(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$*
	chmod 644 $(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$*
endif
	install -m644 $(builddir)/build-$*/.config \
		$(pkgdir)/boot/config-$(release)$(debnum)-$*
	install -m644 $(abidir)/$* \
		$(pkgdir)/boot/abi-$(release)$(debnum)-$*
	install -m644 $(builddir)/build-$*/System.map \
		$(pkgdir)/boot/System.map-$(release)$(debnum)-$*
	$(kmake) O=$(builddir)/build-$* modules_install \
		INSTALL_MOD_PATH=$(pkgdir)/
	rm -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/build
	rm -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/source
ifeq ($(no_image_strip),)
	find $(pkgdir)/ -name \*.ko -print | xargs strip --strip-debug
endif
	# Some initramfs-tools specific modules
	install -d $(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd
	if [ -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/kernel/drivers/video/vesafb.ko ]; then\
	  ln -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/kernel/drivers/video/vesafb.ko \
		$(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd/; \
	fi

	# Now the image scripts
	install -d $(pkgdir)/DEBIAN
	for script in postinst postrm preinst prerm; do				\
	  sed -e 's/=V/$(release)$(debnum)-$*/g' -e 's/=K/$(install_file)/g'	\
	      -e 's/=L/$(loader)/g'		-e 's@=B@$(build_arch)@g'	\
	       debian/control-scripts/$$script > $(pkgdir)/DEBIAN/$$script;	\
	  chmod 755 $(pkgdir)/DEBIAN/$$script;					\
	done

	# Debug image is simple
ifneq ($(skipdbg),true)
	install -m644 -D $(builddir)/build-$*/vmlinux \
		$(dbgpkgdir)/usr/lib/debug/boot/vmlinux-$(release)$(debnum)-$*
	$(kmake) O=$(builddir)/build-$* modules_install \
		INSTALL_MOD_PATH=$(dbgpkgdir)/usr/lib/debug
	rm -f $(dbgpkgdir)/usr/lib/debug/lib/modules/$(release)$(debnum)-$*/build
	rm -f $(dbgpkgdir)/usr/lib/debug/lib/modules/$(release)$(debnum)-$*/source
	rm -f $(dbgpkgdir)/usr/lib/debug/lib/modules/$(release)$(debnum)-$*/modules.*
	rm -fr $(dbgpkgdir)/usr/lib/debug/lib/firmware
endif

	# The flavour specific headers image
	# XXX Would be nice if we didn't have to dupe the original builddir
	install -m644 -D $(builddir)/build-$*/.config \
		$(hdrdir)/.config
	$(kmake) O=$(hdrdir) silentoldconfig prepare scripts
	# We'll symlink this stuff
	rm -f $(hdrdir)/Makefile
	rm -rf $(hdrdir)/include2
	# Script to symlink everything up
	$(SHELL) debian/scripts/link-headers "$(hdrdir)" "$(basepkg)" \
		"" "$(build_arch)"  "$*"
	# Setup the proper asm symlink
	rm -f $(hdrdir)/include/asm
	ln -s asm-$(asm_link) $(hdrdir)/include/asm
	# The build symlink
	install -d debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*
	ln -s /usr/src/$(basepkg)-$* \
		debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*/build
	# And finally the symvers
	install -m644 $(builddir)/build-$*/Module.symvers \
		$(hdrdir)/Module.symvers

	# Now the header scripts
	install -d $(CURDIR)/debian/$(basepkg)-$*/DEBIAN
	for script in postinst; do						\
	  sed -e 's/=V/$(release)$(debnum)-$*/g' -e 's/=K/$(install_file)/g'	\
		debian/control-scripts/headers-$$script > 			\
			$(CURDIR)/debian/$(basepkg)-$*/DEBIAN/$$script;		\
	  chmod 755 $(CURDIR)/debian/$(basepkg)-$*/DEBIAN/$$script;		\
	done

	# At the end of the package prep, call the tests
	DPKG_ARCH="$(arch)" KERN_ARCH="$(build_arch)" FLAVOUR="$*"	\
	 VERSION="$(release)$(debnum)" REVISION="$(revision)"		\
	 PREV_REVISION="$(prev_revision)" ABI_NUM="$(abinum)"		\
	 PREV_ABI_NUM="$(prev_abinum)" BUILD_DIR="$(builddir)/build-$*"	\
	 INSTALL_DIR="$(pkgdir)" SOURCE_DIR="$(CURDIR)"			\
	 run-parts -v debian/tests


headers_tmp := $(CURDIR)/debian/tmp-headers
headers_dir := $(CURDIR)/debian/linux-libc-dev

hmake := $(MAKE) -C $(CURDIR) O=$(headers_tmp) SUBLEVEL=$(SUBLEVEL) \
	EXTRAVERSION=$(debnum) INSTALL_HDR_PATH=$(headers_tmp)/install \
	SHELL="$(SHELL)"

install-arch-headers:
	dh_testdir
	dh_testroot
	dh_clean -k -plinux-libc-dev

	rm -rf $(headers_tmp)
	install -d $(headers_tmp) $(headers_dir)/usr/include/

	$(hmake) ARCH=$(header_arch) $(defconfig)
	mv $(headers_tmp)/.config $(headers_tmp)/.config.old
	sed -e 's/^# \(CONFIG_MODVERSIONS\) is not set$$/\1=y/' \
	  -e 's/.*CONFIG_LOCALVERSION_AUTO.*/# CONFIG_LOCALVERSION_AUTO is not set/' \
	  $(headers_tmp)/.config.old > $(headers_tmp)/.config
	$(hmake) ARCH=$(header_arch) silentoldconfig
	$(hmake) ARCH=$(header_arch) headers_install

	mv $(headers_tmp)/install/include/asm* \
		$(headers_dir)/usr/include/
	mv $(headers_tmp)/install/include/linux \
		$(headers_dir)/usr/include/

	rm -rf $(headers_tmp)

binary-arch-headers: install-arch-headers
	dh_testdir
	dh_testroot

	dh_installchangelogs -plinux-libc-dev
	dh_installdocs -plinux-libc-dev
	dh_compress -plinux-libc-dev
	dh_fixperms -plinux-libc-dev
	dh_installdeb -plinux-libc-dev
	dh_gencontrol -plinux-libc-dev
	dh_md5sums -plinux-libc-dev
	dh_builddeb -plinux-libc-dev

binary-%: pkgimg = linux-image-$(release)$(debnum)-$*
binary-%: pkghdr = linux-headers-$(release)$(debnum)-$*
binary-%: dbgpkg = linux-image-debug-$(release)$(debnum)-$*
binary-%: install-%
	dh_testdir
	dh_testroot

	dh_installchangelogs -p$(pkgimg)
	dh_installdocs -p$(pkgimg)
	dh_compress -p$(pkgimg)
	dh_fixperms -p$(pkgimg)
	dh_installdeb -p$(pkgimg)
	dh_gencontrol -p$(pkgimg)
	dh_md5sums -p$(pkgimg)
	dh_builddeb -p$(pkgimg) -- -Zbzip2 -z9

	dh_installchangelogs -p$(pkghdr)
	dh_installdocs -p$(pkghdr)
	dh_compress -p$(pkghdr)
	dh_fixperms -p$(pkghdr)
	dh_shlibdeps -p$(pkghdr)
	dh_installdeb -p$(pkghdr)
	dh_gencontrol -p$(pkghdr)
	dh_md5sums -p$(pkghdr)
	dh_builddeb -p$(pkghdr)

ifneq ($(skipdbg),true)
	dh_installchangelogs -p$(dbgpkg)
	dh_installdocs -p$(dbgpkg)
	dh_compress -p$(dbgpkg)
	dh_fixperms -p$(dbgpkg)
	dh_installdeb -p$(dbgpkg)
	dh_gencontrol -p$(dbgpkg)
	dh_md5sums -p$(dbgpkg)
	dh_builddeb -p$(dbgpkg)

	# Hokay...here's where we do a little twiddling...
	mv ../$(dbgpkg)_$(release)-$(revision)_$(arch).deb \
		../$(dbgpkg)_$(release)-$(revision)_$(arch).ddeb
	grep -v '^$(dbgpkg)_.*$$' debian/files > debian/files.new
	mv debian/files.new debian/files
	# Now, the package wont get into the archive, but it will get put
	# into the debug system.
endif

$(stampdir)/stamp-flavours:
	@echo $(flavours) $(custom_flavours) > $@

binary-debs: $(stampdir)/stamp-flavours $(addprefix binary-,$(flavours)) \
		binary-arch-headers

build-debs: $(addprefix build-,$(flavours))

build-arch-deps = build-debs
build-arch-deps += build-custom

build-arch: $(build-arch-deps)

binary-arch-deps = binary-debs
binary-arch-deps += binary-custom
binary-arch-deps += binary-udebs

binary-arch: $(binary-arch-deps)
