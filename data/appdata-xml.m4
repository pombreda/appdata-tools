# appdata-xml.m4
#
# serial 1

dnl APPDATA_XML
dnl Installs and validates AppData XML files.
dnl
dnl Call APPDATA_XML in configure.ac to check for the appdata-validate tool.
dnl Add @APPDATA_XML_RULES@ to a Makefile.am to substitute the make rules. Add
dnl .appdata.xml files to appdata_XML in Makefile.am and they will be validated
dnl at make time, as well as installed to the correct location automatically.
dnl
dnl Adding files to appdata_XML does not distribute them automatically.

AC_DEFUN([APPDATA_XML],
[
  m4_pattern_allow([AM_V_GEN])
  AC_SUBST([appdataxmldir], [${datadir}/appdata])
  AC_PATH_PROG([APPDATA_VALIDATE], [appdata-validate])
  AC_SUBST([APPDATA_VALIDATE])
  if test "x$APPDATA_VALIDATE" = "x"; then
    ifelse([$2],,[AC_MSG_ERROR([appdata-validate not found])],[$2])
  else
    ifelse([$1],,[:],[$1])
  fi

  APPDATA_XML_RULES='
.PHONY : uninstall-appdata-xml install-appdata-xml clean-appdata-xml

mostlyclean-am: clean-appdata-xml

%.appdata.valid: %.appdata.xml
	$(AM_V_GEN) if test -f "$<"; then d=; else d="$(srcdir)/"; fi; $(APPDATA_VALIDATE) $${d}$< && touch [$]@

all-am: $(appdata_XML:.appdata.xml=.appdata.valid)
uninstall-am: uninstall-appdata-xml
install-data-am: install-appdata-xml

.SECONDARY: $(appdata_XML)

install-appdata-xml: $(appdata_XML)
	@$(NORMAL_INSTALL)
	if test -n "$^"; then \
		test -z "$(appdataxmldir)" || $(MKDIR_P) "$(DESTDIR)$(appdataxmldir)"; \
		$(INSTALL_DATA) $^ "$(DESTDIR)$(appdataxmldir)"; \
	fi

uninstall-appdata-xml:
	@$(NORMAL_UNINSTALL)
	@list='\''$(appdata_XML)'\''; test -n "$(appdataxmldir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e '\''s|^.*/||'\''`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '\''$(DESTDIR)$(appdataxmldir)'\'' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(appdataxmldir)" && rm -f $$files

clean-appdata-xml:
	rm -f $(appdata_XML:.appdata.xml=.appdata.valid)
'
  _APPDATA_XML_SUBST(APPDATA_XML_RULES)
])

dnl _APPDATA_XML_SUBST(VARIABLE)
dnl Abstract macro to do either _AM_SUBST_NOTMAKE or AC_SUBST
AC_DEFUN([_APPDATA_XML_SUBST],
[
AC_SUBST([$1])
m4_ifdef([_AM_SUBST_NOTMAKE], [_AM_SUBST_NOTMAKE([$1])])
]
)
