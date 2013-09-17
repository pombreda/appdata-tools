AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	-DG_LOG_DOMAIN=\"AppDataTools\"

bin_PROGRAMS =						\
	appdata-validate

appdata_validate_SOURCES =				\
	appdata-validate.c

appdata_validate_LDADD =				\
	$(GLIB_LIBS)

appdata_validate_CFLAGS =				\
	$(WARNINGFLAGS_C)

clean-local:
	rm -f *~

MAINTAINERCLEANFILES =					\
	*~			      			\
	Makefile.in

-include $(top_srcdir)/git.mk
