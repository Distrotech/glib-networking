include $(top_srcdir)/Makefile.decl

giomodule_LTLIBRARIES = libgionss.la

libgionss_la_SOURCES =			\
	nss-module.c			\
	gtlsbackend-nss.c		\
	gtlsbackend-nss.h		\
	gtlscertificate-nss.c		\
	gtlscertificate-nss.h		\
	gtlsclientconnection-nss.c	\
	gtlsclientconnection-nss.h	\
	gtlsconnection-nss.c		\
	gtlsconnection-nss.h		\
	gtlsdatabase-nss.c		\
	gtlsdatabase-nss.h		\
	gtlsfiledatabase-nss.c		\
	gtlsfiledatabase-nss.h		\
	gtlsinputstream-nss.c		\
	gtlsinputstream-nss.h		\
	gtlsoutputstream-nss.c		\
	gtlsoutputstream-nss.h		\
	gtlsserverconnection-nss.c	\
	gtlsserverconnection-nss.h	\
	$(NULL)

INCLUDES += $(NSS_CFLAGS)

libgionss_la_LDFLAGS = $(module_flags)
libgionss_la_LIBADD =			\
	$(GLIB_LIBS)			\
	$(NSS_LIBS)			\
	$(NULL)
