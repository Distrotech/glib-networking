/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <string.h>

#include "gtlscertificate-gnutls.h"
#include <glib/gi18n-lib.h>

static void g_tls_certificate_gnutls_get_property (GObject      *object,
						   guint         prop_id,
						   GValue       *value,
						   GParamSpec   *pspec);
static void g_tls_certificate_gnutls_set_property (GObject      *object,
						   guint         prop_id,
						   const GValue *value,
						   GParamSpec   *pspec);
static void g_tls_certificate_gnutls_finalize     (GObject      *object);

static void     g_tls_certificate_gnutls_initable_iface_init (GInitableIface  *iface);
static gboolean g_tls_certificate_gnutls_initable_init       (GInitable       *initable,
							      GCancellable    *cancellable,
							      GError         **error);

G_DEFINE_TYPE_WITH_CODE (GTlsCertificateGnutls, g_tls_certificate_gnutls, G_TYPE_TLS_CERTIFICATE,
			 G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
						g_tls_certificate_gnutls_initable_iface_init);)

enum
{
  PROP_0,

  PROP_CERTIFICATE,
  PROP_CERTIFICATE_PEM,
  PROP_PRIVATE_KEY,
  PROP_PRIVATE_KEY_PEM
};

struct _GTlsCertificateGnutlsPrivate
{
  gnutls_x509_crt_t cert;
  gnutls_x509_privkey_t key;

  GError *construct_error;

  guint have_cert : 1;
  guint have_key  : 1;
};

static void
g_tls_certificate_gnutls_class_init (GTlsCertificateGnutlsClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsCertificateGnutlsPrivate));

  gobject_class->get_property = g_tls_certificate_gnutls_get_property;
  gobject_class->set_property = g_tls_certificate_gnutls_set_property;
  gobject_class->finalize     = g_tls_certificate_gnutls_finalize;

  g_object_class_override_property (gobject_class, PROP_CERTIFICATE, "certificate");
  g_object_class_override_property (gobject_class, PROP_CERTIFICATE_PEM, "certificate-pem");
  g_object_class_override_property (gobject_class, PROP_PRIVATE_KEY, "private-key");
  g_object_class_override_property (gobject_class, PROP_PRIVATE_KEY_PEM, "private-key-pem");
}

static void
g_tls_certificate_gnutls_finalize (GObject *object)
{
  GTlsCertificateGnutls *gnutls = G_TLS_CERTIFICATE_GNUTLS (object);

  gnutls_x509_crt_deinit (gnutls->priv->cert);
  gnutls_x509_privkey_deinit (gnutls->priv->key);

  g_clear_error (&gnutls->priv->construct_error);

  G_OBJECT_CLASS (g_tls_certificate_gnutls_parent_class)->finalize (object);
}

static void
g_tls_certificate_gnutls_get_property (GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec)
{
  GTlsCertificateGnutls *gnutls = G_TLS_CERTIFICATE_GNUTLS (object);
  GByteArray *certificate;
  char *certificate_pem;
  int status;
  size_t size;

  switch (prop_id)
    {
    case PROP_CERTIFICATE:
      size = 0;
      status = gnutls_x509_crt_export (gnutls->priv->cert,
				       GNUTLS_X509_FMT_DER,
				       NULL, &size);
      if (status != GNUTLS_E_SHORT_MEMORY_BUFFER)
	certificate = NULL;
      else
	{
	  certificate = g_byte_array_sized_new (size);
	  certificate->len = size;
	  status = gnutls_x509_crt_export (gnutls->priv->cert,
					   GNUTLS_X509_FMT_DER,
					   certificate->data, &size);
	  if (status != 0)
	    {
	      g_byte_array_free (certificate, TRUE);
	      certificate = NULL;
	    }
	}
      g_value_take_boxed (value, certificate);
      break;

    case PROP_CERTIFICATE_PEM:
      size = 0;
      status = gnutls_x509_crt_export (gnutls->priv->cert,
				       GNUTLS_X509_FMT_PEM,
				       NULL, &size);
      if (status != GNUTLS_E_SHORT_MEMORY_BUFFER)
	certificate_pem = NULL;
      else
	{
	  certificate_pem = g_malloc (size);
	  status = gnutls_x509_crt_export (gnutls->priv->cert,
					   GNUTLS_X509_FMT_PEM,
					   certificate_pem, &size);
	  if (status != 0)
	    {
	      g_free (certificate_pem);
	      certificate_pem = NULL;
	    }
	}
      g_value_take_string (value, certificate_pem);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_certificate_gnutls_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  GTlsCertificateGnutls *gnutls = G_TLS_CERTIFICATE_GNUTLS (object);
  GByteArray *bytes;
  const char *string;
  gnutls_datum_t data;
  int status;

  switch (prop_id)
    {
    case PROP_CERTIFICATE:
      bytes = g_value_get_boxed (value);
      if (!bytes)
	break;
      g_return_if_fail (gnutls->priv->have_cert == FALSE);
      data.data = bytes->data;
      data.size = bytes->len;
      status = gnutls_x509_crt_import (gnutls->priv->cert, &data,
				       GNUTLS_X509_FMT_DER);
      if (status == 0)
	gnutls->priv->have_cert = TRUE;
      else if (!gnutls->priv->construct_error)
	{
	  gnutls->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse DER certificate: %s"),
			 gnutls_strerror (status));
	}

      break;

    case PROP_CERTIFICATE_PEM:
      string = g_value_get_string (value);
      if (!string)
	break;
      g_return_if_fail (gnutls->priv->have_cert == FALSE);
      data.data = (void *)string;
      data.size = strlen (string);
      status = gnutls_x509_crt_import (gnutls->priv->cert, &data,
				       GNUTLS_X509_FMT_PEM);
      if (status == 0)
	gnutls->priv->have_cert = TRUE;
      else if (!gnutls->priv->construct_error)
	{
	  gnutls->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse PEM certificate: %s"),
			 gnutls_strerror (status));
	}
      break;

    case PROP_PRIVATE_KEY:
      bytes = g_value_get_boxed (value);
      if (!bytes)
	break;
      g_return_if_fail (gnutls->priv->have_key == FALSE);
      data.data = bytes->data;
      data.size = bytes->len;
      status = gnutls_x509_privkey_import (gnutls->priv->key, &data,
					   GNUTLS_X509_FMT_DER);
      if (status == 0)
	gnutls->priv->have_key = TRUE;
      else if (!gnutls->priv->construct_error)
	{
	  gnutls->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse DER private key: %s"),
			 gnutls_strerror (status));
	}
      break;

    case PROP_PRIVATE_KEY_PEM:
      string = g_value_get_string (value);
      if (!string)
	break;
      g_return_if_fail (gnutls->priv->have_key == FALSE);
      data.data = (void *)string;
      data.size = strlen (string);
      status = gnutls_x509_privkey_import (gnutls->priv->key, &data,
					   GNUTLS_X509_FMT_PEM);
      if (status == 0)
	gnutls->priv->have_key = TRUE;
      else if (!gnutls->priv->construct_error)
	{
	  gnutls->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse PEM private key: %s"),
			 gnutls_strerror (status));
	}
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_certificate_gnutls_init (GTlsCertificateGnutls *gnutls)
{
  gnutls->priv = G_TYPE_INSTANCE_GET_PRIVATE (gnutls,
					      G_TYPE_TLS_CERTIFICATE_GNUTLS,
					      GTlsCertificateGnutlsPrivate);

  gnutls_x509_crt_init (&gnutls->priv->cert);
  gnutls_x509_privkey_init (&gnutls->priv->key);
}

static void
g_tls_certificate_gnutls_initable_iface_init (GInitableIface  *iface)
{
  iface->init = g_tls_certificate_gnutls_initable_init;
}

static gboolean
g_tls_certificate_gnutls_initable_init (GInitable       *initable,
					GCancellable    *cancellable,
					GError         **error)
{
  GTlsCertificateGnutls *gnutls = G_TLS_CERTIFICATE_GNUTLS (initable);

  if (gnutls->priv->construct_error)
    {
      g_propagate_error (error, gnutls->priv->construct_error);
      gnutls->priv->construct_error = NULL;
      return FALSE;
    }
  else if (!gnutls->priv->have_cert)
    {
      g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			   _("No certificate data provided"));
      return FALSE;
    }
  else
    return TRUE;
}

GTlsCertificate *
g_tls_certificate_gnutls_new (const gnutls_datum *datum,
			      GTlsCertificate    *issuer)
{
  GTlsCertificateGnutls *gnutls;

  gnutls = g_object_new (G_TYPE_TLS_CERTIFICATE_GNUTLS,
			 "issuer", issuer,
			 NULL);
  if (gnutls_x509_crt_import (gnutls->priv->cert, datum,
			      GNUTLS_X509_FMT_DER) == 0)
    gnutls->priv->have_cert = TRUE;

  return G_TLS_CERTIFICATE (gnutls);
}

const gnutls_x509_crt_t
g_tls_certificate_gnutls_get_cert (GTlsCertificateGnutls *gnutls)
{
  return gnutls->priv->cert;
}

const gnutls_x509_privkey_t
g_tls_certificate_gnutls_get_key (GTlsCertificateGnutls *gnutls)
{
  return gnutls->priv->key;
}

gnutls_x509_crt_t
g_tls_certificate_gnutls_copy_cert (GTlsCertificateGnutls *gnutls)
{
  gnutls_x509_crt_t cert;
  gnutls_datum data;
  size_t size;

  size = 0;
  gnutls_x509_crt_export (gnutls->priv->cert, GNUTLS_X509_FMT_DER,
			  NULL, &size);
  data.data = g_malloc (size);
  data.size = size;
  gnutls_x509_crt_export (gnutls->priv->cert, GNUTLS_X509_FMT_DER,
			  data.data, &size);

  gnutls_x509_crt_init (&cert);
  gnutls_x509_crt_import (cert, &data, GNUTLS_X509_FMT_DER);
  g_free (data.data);

  return cert;
}

gnutls_x509_privkey_t
g_tls_certificate_gnutls_copy_key  (GTlsCertificateGnutls *gnutls)
{
  gnutls_x509_privkey_t key;

  gnutls_x509_privkey_init (&key);
  gnutls_x509_privkey_cpy (key, gnutls->priv->key);
  return key;
}
