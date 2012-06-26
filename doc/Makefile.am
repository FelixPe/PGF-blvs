DOC_MODULE=$(PACKAGE)-$(VERSION)
DOC_DIR=$(DESTDIR)$(datadir)/doc/$(DOC_MODULE)

doc: html/index.html

clean-local:
	rm -rf html man latex *~

html/index.html: Doxyfile
	doxygen Doxyfile.in

install-data-local: html/index.html
	$(mkinstalldirs) $(DOC_DIR)/html
	$(mkinstalldirs) $(DESTDIR)$(datadir)/man/man3	
	-(cd html && for f in *.html *.png; do $(INSTALL) -m 644 "$$f" "$(DOC_DIR)/html/$$f"; done) 	
	-(cd man/man3; for f in *.3; do $(INSTALL) -m 644 "$$f" "$(DESTDIR)$(datadir)/man/man3/$$f"; done) 
