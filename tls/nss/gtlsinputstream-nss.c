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
#include "gtlsinputstream-nss.h"

static void g_tls_input_stream_nss_pollable_iface_init (GPollableInputStreamInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GTlsInputStreamNss, g_tls_input_stream_nss, G_TYPE_INPUT_STREAM,
			 G_IMPLEMENT_INTERFACE (G_TYPE_POLLABLE_INPUT_STREAM, g_tls_input_stream_nss_pollable_iface_init)
			 )

struct _GTlsInputStreamNssPrivate
{
  GTlsConnectionNss *conn;

  /* pending operation metadata */
  GCancellable *cancellable;
  gpointer buffer;
  gsize count;
};

static void
g_tls_input_stream_nss_dispose (GObject *object)
{
  GTlsInputStreamNss *stream = G_TLS_INPUT_STREAM_NSS (object);

  if (stream->priv->conn)
    {
      g_object_remove_weak_pointer (G_OBJECT (stream->priv->conn),
				    (gpointer *)&stream->priv->conn);
      stream->priv->conn = NULL;
    }

  G_OBJECT_CLASS (g_tls_input_stream_nss_parent_class)->dispose (object);
}

static gssize
g_tls_input_stream_nss_read (GInputStream  *stream,
			     void          *buffer,
			     gsize          count,
			     GCancellable  *cancellable,
			     GError       **error)
{
  g_return_val_if_reached (-1);
}

static void
g_tls_input_stream_nss_read_async (GInputStream        *stream,
				   void                *buffer,
				   gsize                count,
				   gint                 io_priority,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
  g_return_if_reached ();
}

static gssize
g_tls_input_stream_nss_read_finish (GInputStream  *stream,
				       GAsyncResult  *result,
				       GError       **error)
{
  g_return_val_if_reached (-1);
}

static gboolean
g_tls_input_stream_nss_pollable_is_readable (GPollableInputStream *pollable)
{
  g_return_val_if_reached (FALSE);
}

static GSource *
g_tls_input_stream_nss_pollable_create_source (GPollableInputStream *pollable,
					       GCancellable         *cancellable)
{
  g_return_val_if_reached (NULL);
}

static gssize
g_tls_input_stream_nss_pollable_read_nonblocking (GPollableInputStream  *pollable,
						  void                  *buffer,
						  gsize                  size,
						  GError               **error)
{
  g_return_val_if_reached (-1);
}

static void
g_tls_input_stream_nss_class_init (GTlsInputStreamNssClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *input_stream_class = G_INPUT_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsInputStreamNssPrivate));

  gobject_class->dispose = g_tls_input_stream_nss_dispose;

  input_stream_class->read_fn = g_tls_input_stream_nss_read;
  input_stream_class->read_async = g_tls_input_stream_nss_read_async;
  input_stream_class->read_finish = g_tls_input_stream_nss_read_finish;
}

static void
g_tls_input_stream_nss_pollable_iface_init (GPollableInputStreamInterface *iface)
{
  iface->is_readable = g_tls_input_stream_nss_pollable_is_readable;
  iface->create_source = g_tls_input_stream_nss_pollable_create_source;
  iface->read_nonblocking = g_tls_input_stream_nss_pollable_read_nonblocking;
}

static void
g_tls_input_stream_nss_init (GTlsInputStreamNss *stream)
{
  stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, G_TYPE_TLS_INPUT_STREAM_NSS, GTlsInputStreamNssPrivate);
}

GInputStream *
g_tls_input_stream_nss_new (GTlsConnectionNss *conn)
{
  GTlsInputStreamNss *tls_stream;

  tls_stream = g_object_new (G_TYPE_TLS_INPUT_STREAM_NSS, NULL);
  tls_stream->priv->conn = conn;
  g_object_add_weak_pointer (G_OBJECT (conn),
			     (gpointer *)&tls_stream->priv->conn);

  return G_INPUT_STREAM (tls_stream);
}
