/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc
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
#include "glib.h"

#include <nss.h>
#include <cert.h>
#include <keyhi.h>
#include <secerr.h>

#include "gtlscertificate-nss.h"
#include "gtlsbackend-nss.h"
#include "gtlsdatabase-nss.h"
#include "gtlsfiledatabase-nss.h"
#include <glib/gi18n-lib.h>

static void     g_tls_certificate_nss_initable_iface_init (GInitableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (GTlsCertificateNss, g_tls_certificate_nss, G_TYPE_TLS_CERTIFICATE,
			 G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
						g_tls_certificate_nss_initable_iface_init);)

enum
{
  PROP_0,

  PROP_CERTIFICATE,
  PROP_CERTIFICATE_PEM,
  PROP_PRIVATE_KEY,
  PROP_PRIVATE_KEY_PEM,
  PROP_ISSUER
};

struct _GTlsCertificateNssPrivate
{
  CERTCertificate *cert;
  SECKEYPrivateKey *key;
  gboolean need_private_key;

  GTlsCertificateNss *issuer;
  gboolean expanded;

  GError *construct_error;
};

static GObject *
g_tls_certificate_nss_constructor (GType                  type,
				   guint                  n_construct_properties,
				   GObjectConstructParam *construct_properties)
{
  GObject *obj;
  GTlsCertificateNss *nss, *existing;

  obj = G_OBJECT_CLASS (g_tls_certificate_nss_parent_class)->constructor (type, n_construct_properties, construct_properties);
  nss = G_TLS_CERTIFICATE_NSS (obj);

  /* If it's invalid, return it as-is */
  if (nss->priv->construct_error || !nss->priv->cert)
    return obj;

  /* If we already have a GTlsCertificate for this CERTCertificate,
   * return that instead.
   */
  existing = g_tls_database_nss_get_gcert (g_tls_backend_nss_default_database,
					   nss->priv->cert, FALSE);
  if (existing)
    {
      if (nss->priv->cert)
	{
	  CERT_DestroyCertificate (nss->priv->cert);
	  nss->priv->cert = NULL;
	}
      if (nss->priv->key && !existing->priv->key)
	{
	  existing->priv->key = nss->priv->key;
	  nss->priv->key = NULL;
	}
      if (nss->priv->issuer && !existing->priv->issuer)
	{
	  existing->priv->issuer = nss->priv->issuer;
	  nss->priv->issuer = NULL;
	}

      g_object_unref (obj);
      obj = G_OBJECT (existing);
    }
  else
    {
      g_tls_database_nss_gcert_created (g_tls_backend_nss_default_database,
					nss->priv->cert, nss);
    }

  return obj;
}

static void
g_tls_certificate_nss_finalize (GObject *object)
{
  GTlsCertificateNss *nss = G_TLS_CERTIFICATE_NSS (object);

  if (nss->priv->cert)
    {
      g_tls_database_nss_gcert_destroyed (g_tls_backend_nss_default_database,
					  nss->priv->cert);
      CERT_DestroyCertificate (nss->priv->cert);
    }
  if (nss->priv->key)
    SECKEY_DestroyPrivateKey (nss->priv->key);

  if (nss->priv->issuer)
    g_object_unref (nss->priv->issuer);

  g_clear_error (&nss->priv->construct_error);

  G_OBJECT_CLASS (g_tls_certificate_nss_parent_class)->finalize (object);
}

#define PEM_CERTIFICATE_HEADER "-----BEGIN CERTIFICATE-----"
#define PEM_CERTIFICATE_FOOTER "-----END CERTIFICATE-----"
#define PEM_PRIVKEY_HEADER     "-----BEGIN RSA PRIVATE KEY-----"
#define PEM_PRIVKEY_FOOTER     "-----END RSA PRIVATE KEY-----"

static char *
encode_pem (const char *header,
	    const char *footer,
	    guint8     *data,
	    gsize       length)
{
  GString *pem;
  char *out;
  int encoded_len, broken_len, full_len;
  int state = 0, save = 0;

  encoded_len = (length / 3 + 1) * 4;
  broken_len = encoded_len + (encoded_len / 72) + 1;
  full_len = strlen (header) + 1 + broken_len + strlen (footer) + 1;

  pem = g_string_sized_new (full_len + 1);
  g_string_append (pem, header);
  g_string_append_c (pem, '\n');
  out = pem->str + pem->len;
  out += g_base64_encode_step (data, length, TRUE, out, &state, &save);
  out += g_base64_encode_close (TRUE, out, &state, &save);
  pem->len = out - pem->str;
  g_string_append (pem, footer);
  g_string_append_c (pem, '\n');

  return g_string_free (pem, FALSE);
}

static void
g_tls_certificate_nss_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
  GTlsCertificateNss *nss = G_TLS_CERTIFICATE_NSS (object);
  CERTCertificate *nss_cert = nss->priv->cert;

  switch (prop_id)
    {
    case PROP_CERTIFICATE:
      if (nss_cert)
	{
	  GByteArray *certificate;

	  certificate = g_byte_array_sized_new (nss_cert->derCert.len);
	  certificate->len = nss_cert->derCert.len;
	  memcpy (certificate->data, nss_cert->derCert.data,
		  nss_cert->derCert.len);
	  g_value_take_boxed (value, certificate);
	}
      else
	g_value_set_boxed (value, NULL);
      break;

    case PROP_CERTIFICATE_PEM:
      if (nss_cert)
	{
	  g_value_take_string (value, encode_pem (PEM_CERTIFICATE_HEADER,
						  PEM_CERTIFICATE_FOOTER,
						  nss_cert->derCert.data,
						  nss_cert->derCert.len));
	}
      else
	g_value_set_string (value, NULL);
      break;

    case PROP_ISSUER:
      g_value_set_object (value, nss->priv->issuer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_certificate_nss_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
  GTlsCertificateNss *nss = G_TLS_CERTIFICATE_NSS (object);
  GByteArray *bytes;
  const gchar *string;

  switch (prop_id)
    {
    case PROP_CERTIFICATE:
      bytes = g_value_get_boxed (value);
      if (!bytes)
	break;
      g_return_if_fail (nss->priv->cert == NULL);

      /* Make sure it's really DER */
      if (!g_strstr_len ((gchar *)bytes->data, bytes->len, "BEGIN CERTIFICATE"))
	nss->priv->cert = CERT_DecodeCertFromPackage ((gchar *)bytes->data, bytes->len);

      if (!nss->priv->cert && !nss->priv->construct_error)
	{
	  nss->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse DER certificate"));
	}
      break;

    case PROP_CERTIFICATE_PEM:
      string = g_value_get_string (value);
      if (!string)
	break;
      g_return_if_fail (nss->priv->cert == NULL);

      /* Make sure it's really PEM */
      if (strstr (string, "BEGIN CERTIFICATE"))
	nss->priv->cert = CERT_DecodeCertFromPackage ((gchar *)string, strlen (string));

      if (!nss->priv->cert && !nss->priv->construct_error)
	{
	  nss->priv->construct_error =
	    g_error_new (G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			 _("Could not parse PEM certificate"));
	}
      break;

    case PROP_PRIVATE_KEY:
    case PROP_PRIVATE_KEY_PEM:
      /* FIXME! */
      break;

    case PROP_ISSUER:
      nss->priv->issuer = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_certificate_nss_init (GTlsCertificateNss *nss)
{
  nss->priv = G_TYPE_INSTANCE_GET_PRIVATE (nss,
					   G_TYPE_TLS_CERTIFICATE_NSS,
					   GTlsCertificateNssPrivate);
}

static gboolean
g_tls_certificate_nss_initable_init (GInitable       *initable,
				     GCancellable    *cancellable,
				     GError         **error)
{
  GTlsCertificateNss *nss = G_TLS_CERTIFICATE_NSS (initable);

  if (nss->priv->construct_error)
    {
      g_propagate_error (error, nss->priv->construct_error);
      nss->priv->construct_error = NULL;
      return FALSE;
    }
  else if (!nss->priv->cert)
    {
      g_set_error_literal (error, G_TLS_ERROR, G_TLS_ERROR_BAD_CERTIFICATE,
			   _("No certificate data provided"));
      return FALSE;
    }
  else
    return TRUE;
}

/* FIXME: mostly dup from gnutls */
static GTlsCertificateFlags
g_tls_certificate_nss_verify_identity (GTlsCertificateNss *nss,
				       GSocketConnectable *identity)
{
  const char *hostname;

  if (G_IS_NETWORK_ADDRESS (identity))
    hostname = g_network_address_get_hostname (G_NETWORK_ADDRESS (identity));
  else if (G_IS_NETWORK_SERVICE (identity))
    hostname = g_network_service_get_domain (G_NETWORK_SERVICE (identity));
  else
    hostname = NULL;

  if (hostname)
    {
      if (CERT_VerifyCertName (nss->priv->cert, hostname) == SECSuccess)
	return 0;
    }

  /* FIXME: check sRVName and uniformResourceIdentifier
   * subjectAltNames, if appropriate for @identity.
   */

  return G_TLS_CERTIFICATE_BAD_IDENTITY;
}

static void
g_tls_certificate_nss_expand_chain (GTlsCertificateNss *nss_cert)
{
  GTlsCertificateNss *c, *issuer = NULL;
  CERTCertificateList *list;
  CERTCertificate *cert;
  SECCertUsage usage;
  int i;

  g_return_if_fail (nss_cert->priv->cert->nsCertType != 0);

  if (nss_cert->priv->expanded)
    return;

  if (nss_cert->priv->cert->nsCertType & NS_CERT_TYPE_SSL_CLIENT)
    usage = certUsageSSLClient;
  else
    usage = certUsageSSLServer;

  list = CERT_CertChainFromCert (nss_cert->priv->cert, usage, PR_TRUE);
  /* list->certs[0] is nss_cert itself, so start from index 1 */
  for (i = 1, c = nss_cert; i < list->len; i++, c = c->priv->issuer)
    {
      cert = CERT_FindCertByDERCert (g_tls_backend_nss_certdbhandle,
				     &list->certs[i]);
      issuer = g_tls_database_nss_get_gcert (g_tls_backend_nss_default_database, cert, TRUE);
      CERT_DestroyCertificate (cert);

      if (c->priv->issuer == issuer)
	{
	  g_object_unref (issuer);
	  break;
	}

      if (c->priv->issuer)
	g_object_unref (c->priv->issuer);
      c->priv->issuer = issuer;
      c->priv->expanded = TRUE;
    }

  if (i == list->len && issuer)
    {
      issuer->priv->expanded = TRUE;
      g_clear_object (&issuer->priv->issuer);
    }

  CERT_DestroyCertificateList (list);
}

/* Our certificate verification routine... called by both
 * g_tls_certificate_nss_verify() and g_tls_database_nss_verify_chain().
 *
 * For our verification purposes, we have to treat the certificates in
 * @database or @trusted_ca as though they were trusted, but we can't
 * actually mark them trusted because we don't know why our caller is
 * currently considering them trusted, so we can't let that trust
 * "leak" into the rest of the program.
 *
 * Fortunately, NSS will tell us in excruciating detail exactly what it
 * doesn't like about a certificate, and so if the only problem with
 * the cert is that it's signed by a @database or @trusted_ca cert that
 * NSS doesn't like, then we can just ignore that error.
 */
GTlsCertificateFlags
g_tls_certificate_nss_verify_full (GTlsCertificate          *chain,
				   GTlsDatabase             *database,
				   GTlsCertificate          *trusted_ca,
				   const gchar              *purpose,
				   GSocketConnectable       *identity,
				   GTlsInteraction          *interaction,
				   GTlsDatabaseVerifyFlags   flags,
				   GCancellable             *cancellable,
				   GError                  **error)
{
  GTlsCertificateNss *nss_cert = G_TLS_CERTIFICATE_NSS (chain);
  GTlsCertificateFlags result;
  SECCertificateUsage usage;
  PLArenaPool *arena;
  CERTVerifyLog *log;
  CERTVerifyLogNode *node;
  PRTime now = PR_Now ();
  SECCertTimeValidity time_validity;
  int trusted_ca_index = -1;

  g_return_val_if_fail (database == NULL || trusted_ca == NULL, G_TLS_CERTIFICATE_GENERIC_ERROR);

  if (database == (GTlsDatabase *)g_tls_backend_nss_default_database)
    database = NULL;

  if (!strcmp (purpose, G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER))
    usage = certificateUsageSSLServer;
  else if (!strcmp (purpose, G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT))
    usage = certificateUsageSSLClient;
  else
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
		   _("Unsupported key purpose OID '%s'"), purpose);
      return G_TLS_CERTIFICATE_GENERIC_ERROR;
    }

  result = 0;

  /* Verify the certificate and log all errors. As a side effect, this
   * will ensure that nss_cert->priv->cert->nsCertType is set.
   */
  arena = PORT_NewArena (512);
  log = PORT_ArenaZNew (arena, CERTVerifyLog);
  log->arena = arena;
  CERT_VerifyCert (g_tls_backend_nss_certdbhandle, nss_cert->priv->cert,
		   PR_TRUE, usage, now, interaction, log);

  /* Now expand the gtls-level chain, and see if it contains a cert
   * from @trusted_ca or @database, and if so, remember where in the
   * chain it is.
   */
  g_tls_certificate_nss_expand_chain (nss_cert);
  if (database || trusted_ca)
    {
      GTlsFileDatabaseNss *db_nss = database ? G_TLS_FILE_DATABASE_NSS (database) : NULL;
      GTlsCertificateNss *c;
      int n;

      for (c = nss_cert, n = 0; c; c = c->priv->issuer, n++)
	{
	  if (trusted_ca && c == (GTlsCertificateNss *)trusted_ca)
	    break;
	  if (db_nss && g_tls_file_database_nss_contains (db_nss, c))
	    break;
	}

      if (c)
	trusted_ca_index = n;
      else
	result |= G_TLS_CERTIFICATE_UNKNOWN_CA;
    }

  /* Now go through the verification log translating the errors */
  for (node = log->head; node; node = node->next)
    {
      if (trusted_ca_index != -1 && node->depth > trusted_ca_index)
	break;

      switch (node->error)
	{
	case SEC_ERROR_INADEQUATE_KEY_USAGE:
	  /* Cert is not appropriately tagged for signing. For
	   * historical/compatibility reasons, we ignore this when
	   * using PEM certificates.
	   */
	  if (database || trusted_ca)
	    break;
	  /* else fall through */

	case SEC_ERROR_UNKNOWN_ISSUER:
	  /* Cert was issued by an unknown CA */
	case SEC_ERROR_UNTRUSTED_ISSUER:
	  /* Cert is a CA that is not marked trusted */
	case SEC_ERROR_CA_CERT_INVALID:
	  /* Cert is not a CA */

	  /* These are all OK if they occur on the trusted CA, but not
	   * before it.
	   */
	  if (node->depth != trusted_ca_index)
	    result |= G_TLS_CERTIFICATE_UNKNOWN_CA;
	  break;

	case SEC_ERROR_CERT_NOT_IN_NAME_SPACE:
	  /* Cert is not authorized to sign the cert it signed */
	  result |= G_TLS_CERTIFICATE_UNKNOWN_CA;
	  break;

	case SEC_ERROR_EXPIRED_CERTIFICATE:
	case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
	  /* Cert is either expired or not yet valid;
	   * CERT_VerifyCert() doesn't distinguish.
	   */
	  time_validity =  CERT_CheckCertValidTimes (node->cert, now, PR_FALSE);
	  if (time_validity == secCertTimeNotValidYet)
	    result |= G_TLS_CERTIFICATE_NOT_ACTIVATED;
	  else if (time_validity == secCertTimeExpired)
	    result |= G_TLS_CERTIFICATE_EXPIRED;
	  break;

	case SEC_ERROR_REVOKED_CERTIFICATE:
	  result |= G_TLS_CERTIFICATE_REVOKED;
	  break;

	default:
	  result |= G_TLS_CERTIFICATE_GENERIC_ERROR;
	  break;
	}

      CERT_DestroyCertificate (node->cert);
    }

  for (; node; node = node->next)
    CERT_DestroyCertificate (node->cert);
  PORT_FreeArena (log->arena, PR_FALSE);

  if (identity)
    result |= g_tls_certificate_nss_verify_identity (nss_cert, identity);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    result = G_TLS_CERTIFICATE_GENERIC_ERROR;

  return result;
}

static GTlsCertificateFlags
g_tls_certificate_nss_verify (GTlsCertificate     *cert,
			      GSocketConnectable  *identity,
			      GTlsCertificate     *trusted_ca)
{
  GTlsCertificateNss *nss_cert = G_TLS_CERTIFICATE_NSS (cert);
  GTlsCertificateFlags flags;

  /* nss_cert->priv->cert->nsCertType may not have been set yet, but
   * it will get set as a side effect of verifying the cert. If we
   * don't know yet what kind of key it is, we'll try server first.
   */

  if ((nss_cert->priv->cert->nsCertType & NS_CERT_TYPE_SSL_SERVER) ||
      (nss_cert->priv->cert->nsCertType == 0))
    {
      flags = g_tls_certificate_nss_verify_full (cert, NULL, trusted_ca,
						 G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER,
						 identity, NULL, 0, NULL, NULL);
    }

  if (!(nss_cert->priv->cert->nsCertType & NS_CERT_TYPE_SSL_SERVER))
    {
      flags = g_tls_certificate_nss_verify_full (cert, NULL, trusted_ca,
						 G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT,
						 identity, NULL, 0, NULL, NULL);
    }

  return flags;
}

static void
g_tls_certificate_nss_class_init (GTlsCertificateNssClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GTlsCertificateClass *certificate_class = G_TLS_CERTIFICATE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsCertificateNssPrivate));

  gobject_class->constructor  = g_tls_certificate_nss_constructor;
  gobject_class->get_property = g_tls_certificate_nss_get_property;
  gobject_class->set_property = g_tls_certificate_nss_set_property;
  gobject_class->finalize     = g_tls_certificate_nss_finalize;

  certificate_class->verify = g_tls_certificate_nss_verify;

  g_object_class_override_property (gobject_class, PROP_CERTIFICATE, "certificate");
  g_object_class_override_property (gobject_class, PROP_CERTIFICATE_PEM, "certificate-pem");
  g_object_class_override_property (gobject_class, PROP_PRIVATE_KEY, "private-key");
  g_object_class_override_property (gobject_class, PROP_PRIVATE_KEY_PEM, "private-key-pem");
  g_object_class_override_property (gobject_class, PROP_ISSUER, "issuer");
}

static void
g_tls_certificate_nss_initable_iface_init (GInitableIface  *iface)
{
  iface->init = g_tls_certificate_nss_initable_init;
}

GTlsCertificateNss *
g_tls_certificate_nss_new_for_cert (CERTCertificate *cert)
{
  GTlsCertificateNss *nss;

  nss = g_object_new (G_TYPE_TLS_CERTIFICATE_NSS, NULL);
  nss->priv->cert = CERT_DupCertificate (cert);

  return nss;
}

CERTCertificate *
g_tls_certificate_nss_get_cert (GTlsCertificateNss *nss)
{
  return nss->priv->cert;
}


