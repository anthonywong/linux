
indep_hdrpkg = $(hdrs_pkg_name)
indep_hdrdir = $(CURDIR)/debian/$(indep_hdrpkg)/usr/src/$(indep_hdrpkg)
install-headers:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(indep_hdrpkg)

	install -d $(indep_hdrdir)
	find . -path './debian' -prune -o -path './$(DEBIAN)' -prune \
	  -o -path './include/*' -prune \
	  -o -path './scripts/*' -prune -o -type f \
	  \( -name 'Makefile*' -o -name 'Kconfig*' -o -name 'Kbuild*' -o \
	     -name '*.sh' -o -name '*.pl' -o -name '*.lds' \) \
	  -print | cpio -pd --preserve-modification-time $(indep_hdrdir)
	cp -a drivers/media/dvb/dvb-core/*.h $(indep_hdrdir)/drivers/media/dvb/dvb-core
	cp -a drivers/media/video/*.h $(indep_hdrdir)/drivers/media/video
	cp -a drivers/media/dvb/frontends/*.h $(indep_hdrdir)/drivers/media/dvb/frontends
	cp -a scripts include $(indep_hdrdir)
	(find arch -name include -type d -print | \
		xargs -n1 -i: find : -type f) | \
		cpio -pd --preserve-modification-time $(indep_hdrdir)

build-indep:
binary-indep:
