AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	$(APPSTREAM_CFLAGS)				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"		\
	-DG_LOG_DOMAIN=\"AppDataTools\"

bin_PROGRAMS =						\
	appdata-validate

appdata_validate_SOURCES =				\
	appdata-validate.c

appdata_validate_LDADD =				\
	$(APPSTREAM_LIBS)				\
	$(GLIB_LIBS)

appdata_validate_CFLAGS =				\
	$(WARNINGFLAGS_C)

clean-local:
	rm -f *~

MAINTAINERCLEANFILES =					\
	*~						\
	*.trs						\
	*.log						\
	Makefile.in

-include $(top_srcdir)/git.mk
