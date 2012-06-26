
SUBDIRS = src include

if HAS_DOXYGEN
SUBDIRS += doc
endif

PACKAGE		= @PACKAGE@
VERSION		= @VERSION@
EXTRA_DIST	= autogen.sh FAQ PGFCodec.sln PGFCodec.vcproj \
		  libpgf.spec
#EXTRA_DIST 	= TODO BUGS README ChangeLog ReleaseNotes $(PACKAGE).spec \
#	          $(PACKAGE).spec.in $(PACKAGE).pc $(PACKAGE).pc.in
ARCH		= i386
RPM_DIR		= $(HOME)/redhat
DISTCLEANFILES  = $(PACKAGE)-$(VERSION).tar.gz *~

RPMEXTRACONFOPT="--disable-dependency-tracking"

RPMRELEASE=$(PACKAGE_RELEASE)

install-data-local:: $(PACKAGE).pc
	$(mkinstalldirs) $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 644 $(PACKAGE).pc $(DESTDIR)$(libdir)/pkgconfig/$(PACKAGE).pc


.PHONY: ChangeLog
ChangeLog:
	echo "ChangeLog generation disabled" #	svn2cl.sh


rpm:  $(RPM_DIR) dist 
	rpmbuild --clean --quiet \
	   --define='extra_configure_options $(RPMEXTRACONFOPT)' \
	   --define='user $(USER)' \
	   --define='_topdir $(RPM_DIR)' \
	   --define='rpm_release $(RPMRELEASE)' \
	   --define='rpm_testbuild 0' \
	   $(RPMEXTRADEFINES) \
	   $(RPMEXTRAARGS) \
	   -ta $(PACKAGE)-$(VERSION).tar.gz

$(RPM_DIR):
	-mkdir  $(RPM_DIR)
	-mkdir  $(RPM_DIR)/BUILD
	-mkdir  $(RPM_DIR)/SPECS
	-mkdir  $(RPM_DIR)/SOURCES
	-mkdir  $(RPM_DIR)/SRPMS
	-mkdir  $(RPM_DIR)/RPMS
	-mkdir  $(RPM_DIR)/RPMS/i386
	-mkdir  $(RPM_DIR)/RPMS/i486
	-mkdir  $(RPM_DIR)/RPMS/i686
	-mkdir  $(RPM_DIR)/RPMS/ia64
	-mkdir  $(RPM_DIR)/RPMS/noarch
