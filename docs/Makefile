# http://www.thaiopensource.com/relaxng/trang.html
TRANG=trang

all: appdata.rng

# all: appdata.rng appdata.xsd appdata.dtd
#
# Unfortunately, generating DTD doesn't work and the generated XSD
# seems to be far too allowing. 

appdata.rng: appdata.rnc
	$(TRANG) $< $@

appdata.xsd: appdata.rnc
	$(TRANG) $< $@

appdata.dtd: appdata.rnc
	$(TRANG) $< $@

