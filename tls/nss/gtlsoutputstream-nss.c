/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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
#include "gtlsoutputstream-nss.h"

static void g_tls_output_stream_nss_pollable_iface_init (GPollableOutputStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GTlsOutputStreamNss, g_tls_output_stream_nss, G_TYPE_OUTPUT_STREAM,
			 G_IMPLEMENT_INTERFACE (G_TYPE_POLLABLE_OUTPUT_STREAM, g_tls_output_stream_nss_pollable_iface_init)
			 )

struct _GTlsOutputStreamNssPrivate
{
  GTlsConnectionNss *conn;

  /* pending operation metadata */
  GCancellable *cancellable;
  gconstpointer buffer;
  gsize count;
};

static void
g_tls_output_stream_nss_dispose (GObject *object)
{
  GTlsOutputStreamNss *stream = G_TLS_OUTPUT_STREAM_NSS (object);

  if (stream->priv->conn)
    {
      g_object_remove_weak_pointer (G_OBJECT (stream->priv->conn),
				    (gpointer *)&stream->priv->conn);
      stream->priv->conn = NULL;
    }

  G_OBJECT_CLASS (g_tls_output_stream_nss_parent_class)->dispose (object);
}

static gssize
g_tls_output_stream_nss_write (GOutputStream  *stream,
			       const void     *buffer,
			       gsize           count,
			       GCancellable   *cancellable,
			       GError        **error)
{
  g_return_val_if_reached (-1);
}

static void
g_tls_output_stream_nss_write_async (GOutputStream        *stream,
				     const void           *buffer,
				     gsize                 count,
				     gint                  io_priority,
				     GCancellable         *cancellable,
				     GAsyncReadyCallback   callback,
				     gpointer              user_data)
{
  g_return_if_reached ();
}

static gssize
g_tls_output_stream_nss_write_finish (GOutputStream  *stream,
				      GAsyncResult   *result,
				      GError        **error)
{
  g_return_val_if_reached (-1);
}

static gboolean
g_tls_output_stream_nss_pollable_is_writable (GPollableOutputStream *pollable)
{
  g_return_val_if_reached (FALSE);
}

static GSource *
g_tls_output_stream_nss_pollable_create_source (GPollableOutputStream *pollable,
						GCancellable         *cancellable)
{
  g_return_val_if_reached (NULL);
}

static gssize
g_tls_output_stream_nss_pollable_write_nonblocking (GPollableOutputStream  *pollable,
						    const void             *buffer,
						    gsize                   size,
						    GError                **error)
{
  g_return_val_if_reached (-1);
}

static void
g_tls_output_stream_nss_class_init (GTlsOutputStreamNssClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GOutputStreamClass *output_stream_class = G_OUTPUT_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsOutputStreamNssPrivate));

  gobject_class->dispose = g_tls_output_stream_nss_dispose;

  output_stream_class->write_fn = g_tls_output_stream_nss_write;
  output_stream_class->write_async = g_tls_output_stream_nss_write_async;
  output_stream_class->write_finish = g_tls_output_stream_nss_write_finish;
}

static void
g_tls_output_stream_nss_pollable_iface_init (GPollableOutputStreamInterface *iface)
{
  iface->is_writable = g_tls_output_stream_nss_pollable_is_writable;
  iface->create_source = g_tls_output_stream_nss_pollable_create_source;
  iface->write_nonblocking = g_tls_output_stream_nss_pollable_write_nonblocking;
}

static void
g_tls_output_stream_nss_init (GTlsOutputStreamNss *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, G_TYPE_TLS_OUTPUT_STREAM_NSS, GTlsOutputStreamNssPrivate);
}

GOutputStream *
g_tls_output_stream_nss_new (GTlsConnectionNss *conn)
{
  GTlsOutputStreamNss *tls_stream;

  tls_stream = g_object_new (G_TYPE_TLS_OUTPUT_STREAM_NSS, NULL);
  tls_stream->priv->conn = conn;
  g_object_add_weak_pointer (G_OBJECT (conn),
			     (gpointer *)&tls_stream->priv->conn);

  return G_OUTPUT_STREAM (tls_stream);
}
