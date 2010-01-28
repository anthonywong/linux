build-indep:

docpkg = $(doc_pkg_name)
docdir = $(CURDIR)/debian/$(docpkg)/usr/share/doc/$(docpkg)
install-doc:
ifeq ($(do_doc_package),true)
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
endif

install-indep: install-doc

binary-indep: install-indep
