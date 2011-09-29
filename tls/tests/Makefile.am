include $(top_srcdir)/Makefile.decl

INCLUDES +=					\
	-I$(top_srcdir)/tls			\
	-DSRCDIR=\""$(abs_srcdir)"\"		\
	-DTOP_BUILDDIR=\""$(top_builddir)"\"

noinst_PROGRAMS = $(TEST_PROGS)

LDADD  = \
	$(GLIB_LIBS)

TEST_PROGS +=			\
	certificate-gnutls	\
	certificate-nss		\
	connection-gnutls	\
	connection-nss		\
	$(NULL)

certificate_gnutls_SOURCES = certificate.c
certificate_gnutls_CPPFLAGS = -DBACKEND=\""gnutls"\"

connection_gnutls_SOURCES = connection.c
connection_gnutls_CPPFLAGS = -DBACKEND=\""gnutls"\"

certificate_nss_SOURCES = certificate.c
certificate_nss_CPPFLAGS = -DBACKEND=\""nss"\"

connection_nss_SOURCES = connection.c
connection_nss_CPPFLAGS = -DBACKEND=\""nss"\"

EXTRA_DIST += \
	files \
	$(NULL)

DISTCLEANFILES = \
	$(NULL)
