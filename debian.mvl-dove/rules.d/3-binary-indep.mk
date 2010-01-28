build-indep:

docpkg = $(doc_pkg_name)
docdir = $(CURDIR)/debian/$(docpkg)/usr/share/doc/$(docpkg)
install-doc:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(docpkg)

	install -d $(docdir)
ifeq ($(do_doc_package_content),true)
	# First the html docs. We skip these for autobuilds
	if [ -z "$(AUTOBUILD)" ]; then \
		install -d $(docdir)/$(doc_pkg_name)-tmp; \
		$(kmake) O=$(docdir)/$(doc_pkg_name)-tmp htmldocs; \
		mv $(docdir)/$(doc_pkg_name)-tmp/Documentation/DocBook \
			$(docdir)/html; \
		rm -rf $(docdir)/$(doc_pkg_name)-tmp; \
	fi
endif
	# Copy the rest
	cp -a Documentation/* $(docdir)
	rm -rf $(docdir)/DocBook
	find $(docdir) -name .gitignore | xargs rm -f

srcpkg = $(src_pkg_name)-source-$(release)
srcdir = $(CURDIR)/debian/$(srcpkg)/usr/src/$(srcpkg)
install-source:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(srcpkg)

	install -d $(srcdir)
ifeq ($(do_linux_source_content),true)
	find . -path './debian' -prune -o -path './$(DEBIAN)' -prune -o \
		-path './.*' -prune -o -print | \
		cpio -pd --preserve-modification-time $(srcdir)
	(cd $(srcdir)/..; tar cf - $(srcpkg)) | bzip2 -9c > \
		$(srcdir).tar.bz2
	rm -rf $(srcdir)
endif

install-indep-deps = 
ifeq ($(do_doc_package),true)
install-indep-deps += install-doc
endif
ifeq ($(do_source_package),true)
install-indep-deps += install-doc
endif
install-indep: $(install-indep-deps)

binary-indep: install-indep
