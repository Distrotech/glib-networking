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
#include <glib.h>

#include "gtlsconnection-nss.h"
#include <glib/gi18n-lib.h>

static void     g_tls_connection_nss_initable_iface_init (GInitableIface  *iface);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GTlsConnectionNss, g_tls_connection_nss, G_TYPE_TLS_CONNECTION,
				  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
							 g_tls_connection_nss_initable_iface_init););


enum
{
  PROP_0,
  PROP_BASE_IO_STREAM,
  PROP_REQUIRE_CLOSE_NOTIFY,
  PROP_REHANDSHAKE_MODE,
  PROP_USE_SYSTEM_CERTDB,
  PROP_DATABASE,
  PROP_CERTIFICATE,
  PROP_INTERACTION,
  PROP_PEER_CERTIFICATE,
  PROP_PEER_CERTIFICATE_ERRORS
};

struct _GTlsConnectionNssPrivate
{
  GIOStream *base_io_stream;
  GPollableInputStream *base_istream;
  GPollableOutputStream *base_ostream;

  GTlsCertificate *certificate, *peer_certificate;
  GTlsCertificateFlags peer_certificate_errors;
  gboolean require_close_notify;
  GTlsRehandshakeMode rehandshake_mode;
  gboolean is_system_certdb;
  GTlsDatabase *database;
  gboolean database_is_unset;

  GInputStream *tls_istream;
  GOutputStream *tls_ostream;

  GTlsInteraction *interaction;
};

static void
g_tls_connection_nss_init (GTlsConnectionNss *nss)
{
  nss->priv = G_TYPE_INSTANCE_GET_PRIVATE (nss, G_TYPE_TLS_CONNECTION_NSS, GTlsConnectionNssPrivate);

  nss->priv->database_is_unset = TRUE;
  nss->priv->is_system_certdb = TRUE;
}

static gboolean
g_tls_connection_nss_initable_init (GInitable     *initable,
				    GCancellable  *cancellable,
				    GError       **error)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (initable);

  g_return_val_if_fail (nss->priv->base_istream != NULL &&
			nss->priv->base_ostream != NULL, FALSE);

  return TRUE;
}

static void
g_tls_connection_nss_finalize (GObject *object)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (object);

  if (nss->priv->base_io_stream)
    g_object_unref (nss->priv->base_io_stream);

  if (nss->priv->tls_istream)
    g_object_unref (nss->priv->tls_istream);
  if (nss->priv->tls_ostream) 
    g_object_unref (nss->priv->tls_ostream);

  if (nss->priv->database)
    g_object_unref (nss->priv->database);
  if (nss->priv->certificate)
    g_object_unref (nss->priv->certificate);
  if (nss->priv->peer_certificate)
    g_object_unref (nss->priv->peer_certificate);

  if (nss->priv->interaction)
    g_object_unref (nss->priv->interaction);

  G_OBJECT_CLASS (g_tls_connection_nss_parent_class)->finalize (object);
}

static void
g_tls_connection_nss_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (object);
  GTlsBackend *backend;

  switch (prop_id)
    {
    case PROP_BASE_IO_STREAM:
      g_value_set_object (value, nss->priv->base_io_stream);
      break;

    case PROP_REQUIRE_CLOSE_NOTIFY:
      g_value_set_boolean (value, nss->priv->require_close_notify);
      break;

    case PROP_REHANDSHAKE_MODE:
      g_value_set_enum (value, nss->priv->rehandshake_mode);
      break;

    case PROP_USE_SYSTEM_CERTDB:
      g_value_set_boolean (value, nss->priv->is_system_certdb);
      break;

    case PROP_DATABASE:
      if (nss->priv->database_is_unset)
        {
          backend = g_tls_backend_get_default ();
          nss->priv->database = g_tls_backend_get_default_database (backend);
          nss->priv->database_is_unset = FALSE;
        }
      g_value_set_object (value, nss->priv->database);
      break;

    case PROP_CERTIFICATE:
      g_value_set_object (value, nss->priv->certificate);
      break;

    case PROP_INTERACTION:
      g_value_set_object (value, nss->priv->interaction);
      break;

    case PROP_PEER_CERTIFICATE:
      g_value_set_object (value, nss->priv->peer_certificate);
      break;

    case PROP_PEER_CERTIFICATE_ERRORS:
      g_value_set_flags (value, nss->priv->peer_certificate_errors);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_connection_nss_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (object);
  GInputStream *istream;
  GOutputStream *ostream;
  gboolean system_certdb;
  GTlsBackend *backend;

  switch (prop_id)
    {
    case PROP_BASE_IO_STREAM:
      if (nss->priv->base_io_stream)
	{
	  g_object_unref (nss->priv->base_io_stream);
	  nss->priv->base_istream = NULL;
	  nss->priv->base_ostream = NULL;
	}
      nss->priv->base_io_stream = g_value_dup_object (value);
      if (!nss->priv->base_io_stream)
	return;

      istream = g_io_stream_get_input_stream (nss->priv->base_io_stream);
      ostream = g_io_stream_get_output_stream (nss->priv->base_io_stream);

      if (G_IS_POLLABLE_INPUT_STREAM (istream) &&
	  g_pollable_input_stream_can_poll (G_POLLABLE_INPUT_STREAM (istream)))
	nss->priv->base_istream = G_POLLABLE_INPUT_STREAM (istream);
      if (G_IS_POLLABLE_OUTPUT_STREAM (ostream) &&
	  g_pollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (ostream)))
	nss->priv->base_ostream = G_POLLABLE_OUTPUT_STREAM (ostream);
      break;

    case PROP_REQUIRE_CLOSE_NOTIFY:
      nss->priv->require_close_notify = g_value_get_boolean (value);
      break;

    case PROP_REHANDSHAKE_MODE:
      nss->priv->rehandshake_mode = g_value_get_enum (value);
      break;

    case PROP_USE_SYSTEM_CERTDB:
      system_certdb = g_value_get_boolean (value);
      if (system_certdb != nss->priv->is_system_certdb)
        {
          g_clear_object (&nss->priv->database);
          if (system_certdb)
            {
              backend = g_tls_backend_get_default ();
              nss->priv->database = g_tls_backend_get_default_database (backend);
            }
          nss->priv->is_system_certdb = system_certdb;
        }
      break;

    case PROP_DATABASE:
      if (nss->priv->database)
	g_object_unref (nss->priv->database);
      nss->priv->database = g_value_dup_object (value);
      nss->priv->is_system_certdb = FALSE;
      nss->priv->database_is_unset = FALSE;
      break;

    case PROP_CERTIFICATE:
      if (nss->priv->certificate)
	g_object_unref (nss->priv->certificate);
      nss->priv->certificate = g_value_dup_object (value);
      break;

    case PROP_INTERACTION:
      if (nss->priv->interaction)
	g_object_unref (nss->priv->interaction);
      nss->priv->interaction = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
g_tls_connection_nss_handshake (GTlsConnection   *conn,
				GCancellable     *cancellable,
				GError          **error)
{
  g_return_val_if_reached (FALSE);
}

static void
g_tls_connection_nss_handshake_async (GTlsConnection       *conn,
				      int                   io_priority,
				      GCancellable         *cancellable,
				      GAsyncReadyCallback   callback,
				      gpointer              user_data)
{
  g_return_if_reached ();
}

static gboolean
g_tls_connection_nss_handshake_finish (GTlsConnection       *conn,
					  GAsyncResult         *result,
					  GError              **error)
{
  g_return_val_if_reached (FALSE);
}

static GInputStream  *
g_tls_connection_nss_get_input_stream (GIOStream *stream)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (stream);

  return nss->priv->tls_istream;
}

static GOutputStream *
g_tls_connection_nss_get_output_stream (GIOStream *stream)
{
  GTlsConnectionNss *nss = G_TLS_CONNECTION_NSS (stream);

  return nss->priv->tls_ostream;
}

static gboolean
g_tls_connection_nss_close (GIOStream     *stream,
			    GCancellable  *cancellable,
			    GError       **error)
{
  g_return_val_if_reached (FALSE);
}

static void
g_tls_connection_nss_close_async (GIOStream           *stream,
				  int                  io_priority,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
  g_return_if_reached ();
}

static gboolean
g_tls_connection_nss_close_finish (GIOStream           *stream,
				      GAsyncResult        *result,
				      GError             **error)
{
  g_return_val_if_reached (FALSE);
}

static void
g_tls_connection_nss_class_init (GTlsConnectionNssClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GTlsConnectionClass *connection_class = G_TLS_CONNECTION_CLASS (klass);
  GIOStreamClass *iostream_class = G_IO_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsConnectionNssPrivate));

  gobject_class->get_property = g_tls_connection_nss_get_property;
  gobject_class->set_property = g_tls_connection_nss_set_property;
  gobject_class->finalize     = g_tls_connection_nss_finalize;

  connection_class->handshake        = g_tls_connection_nss_handshake;
  connection_class->handshake_async  = g_tls_connection_nss_handshake_async;
  connection_class->handshake_finish = g_tls_connection_nss_handshake_finish;

  iostream_class->get_input_stream  = g_tls_connection_nss_get_input_stream;
  iostream_class->get_output_stream = g_tls_connection_nss_get_output_stream;
  iostream_class->close_fn          = g_tls_connection_nss_close;
  iostream_class->close_async       = g_tls_connection_nss_close_async;
  iostream_class->close_finish      = g_tls_connection_nss_close_finish;

  g_object_class_override_property (gobject_class, PROP_BASE_IO_STREAM, "base-io-stream");
  g_object_class_override_property (gobject_class, PROP_REQUIRE_CLOSE_NOTIFY, "require-close-notify");
  g_object_class_override_property (gobject_class, PROP_REHANDSHAKE_MODE, "rehandshake-mode");
  g_object_class_override_property (gobject_class, PROP_USE_SYSTEM_CERTDB, "use-system-certdb");
  g_object_class_override_property (gobject_class, PROP_DATABASE, "database");
  g_object_class_override_property (gobject_class, PROP_CERTIFICATE, "certificate");
  g_object_class_override_property (gobject_class, PROP_INTERACTION, "interaction");
  g_object_class_override_property (gobject_class, PROP_PEER_CERTIFICATE, "peer-certificate");
  g_object_class_override_property (gobject_class, PROP_PEER_CERTIFICATE_ERRORS, "peer-certificate-errors");
}

static void
g_tls_connection_nss_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_tls_connection_nss_initable_init;
}

