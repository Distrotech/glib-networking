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

#include "gtlsserverconnection-nss.h"
#include <glib/gi18n-lib.h>

enum
{
  PROP_0,
  PROP_AUTHENTICATION_MODE
};

static void g_tls_server_connection_nss_server_connection_interface_init (GTlsServerConnectionInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GTlsServerConnectionNss, g_tls_server_connection_nss, G_TYPE_TLS_CONNECTION_NSS,
			 G_IMPLEMENT_INTERFACE (G_TYPE_TLS_SERVER_CONNECTION,
						g_tls_server_connection_nss_server_connection_interface_init))

struct _GTlsServerConnectionNssPrivate
{
  GTlsAuthenticationMode authentication_mode;
};

static void
g_tls_server_connection_nss_init (GTlsServerConnectionNss *nss)
{
  nss->priv = G_TYPE_INSTANCE_GET_PRIVATE (nss, G_TYPE_TLS_SERVER_CONNECTION_NSS, GTlsServerConnectionNssPrivate);
}

static void
g_tls_server_connection_nss_get_property (GObject    *object,
					  guint       prop_id,
					  GValue     *value,
					  GParamSpec *pspec)
{
  GTlsServerConnectionNss *nss = G_TLS_SERVER_CONNECTION_NSS (object);

  switch (prop_id)
    {
    case PROP_AUTHENTICATION_MODE:
      g_value_set_enum (value, nss->priv->authentication_mode);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_server_connection_nss_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec)
{
  GTlsServerConnectionNss *nss = G_TLS_SERVER_CONNECTION_NSS (object);

  switch (prop_id)
    {
    case PROP_AUTHENTICATION_MODE:
      nss->priv->authentication_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tls_server_connection_nss_class_init (GTlsServerConnectionNssClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GTlsServerConnectionNssPrivate));

  gobject_class->get_property = g_tls_server_connection_nss_get_property;
  gobject_class->set_property = g_tls_server_connection_nss_set_property;

  g_object_class_override_property (gobject_class, PROP_AUTHENTICATION_MODE, "authentication-mode");
}

static void
g_tls_server_connection_nss_server_connection_interface_init (GTlsServerConnectionInterface *iface)
{
}

