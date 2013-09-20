
-include $(top_srcdir)/git.mk

man_MANS =
if ENABLE_MAN
man_MANS += appdata-validate.1
endif

xmlschemadir = $(datadir)/appdata/schema
dist_xmlschema_DATA = 				\
	appdata.xsd				\
	schemas.xml				\
	../docs/appdata.rnc

XSLTPROC_FLAGS = \
	--nonet \
	--stringparam man.output.quietly 1 \
	--stringparam funcsynopsis.style ansi \
	--stringparam man.th.extra1.suppress 1 \
	--stringparam man.authors.section.enabled 0 \
	--stringparam man.copyright.section.enabled 0

appdata-validate.1: appdata-validate.xml
	$(AM_V_GEN) xsltproc $(XSLTPROC_FLAGS) http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

edit = sed -e 's|@datadir[@]|$(datadir)|g'

appdata-rng-init.el:
	rm -f $@ $@.tmp
	srcdir=''; \
	    test -f ./$@.in || srcdir=$(srcdir)/; \
	    $(edit) $${srcdir}$@.in >$@.tmp
	mv $@.tmp $@

appdata-rng-init.el: appdata-rng-init.el.in

emacsinitdir = $(datadir)/emacs/site-lisp/site-start.d
dist_emacsinit_DATA = appdata-rng-init.el

EXTRA_DIST =					\
	appdata-validate.xml			\
	broken.appdata.xml			\
	screenshots.appdata.xml			\
	success.appdata.xml			\
	translated.appdata.xml			\
	wrong-extension.xml

clean-local:
	rm -f *~
	rm -f *.1
	rm -f manpage.*
	rm -f *.el

CLEANFILES =						\
	appdata-validate.1				\
	manpage.*
