/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#ifndef __G_TLS_INPUT_STREAM_NSS_H__
#define __G_TLS_INPUT_STREAM_NSS_H__

#include <gio/gio.h>
#include "gtlsconnection-nss.h"

G_BEGIN_DECLS

#define G_TYPE_TLS_INPUT_STREAM_NSS            (g_tls_input_stream_nss_get_type ())
#define G_TLS_INPUT_STREAM_NSS(inst)           (G_TYPE_CHECK_INSTANCE_CAST ((inst), G_TYPE_TLS_INPUT_STREAM_NSS, GTlsInputStreamNss))
#define G_TLS_INPUT_STREAM_NSS_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_TLS_INPUT_STREAM_NSS, GTlsInputStreamNssClass))
#define G_IS_TLS_INPUT_STREAM_NSS(inst)        (G_TYPE_CHECK_INSTANCE_TYPE ((inst), G_TYPE_TLS_INPUT_STREAM_NSS))
#define G_IS_TLS_INPUT_STREAM_NSS_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), G_TYPE_TLS_INPUT_STREAM_NSS))
#define G_TLS_INPUT_STREAM_NSS_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst), G_TYPE_TLS_INPUT_STREAM_NSS, GTlsInputStreamNssClass))

typedef struct _GTlsInputStreamNssPrivate GTlsInputStreamNssPrivate;
typedef struct _GTlsInputStreamNssClass   GTlsInputStreamNssClass;
typedef struct _GTlsInputStreamNss        GTlsInputStreamNss;

struct _GTlsInputStreamNssClass
{
  GInputStreamClass parent_class;
};

struct _GTlsInputStreamNss
{
  GInputStream parent_instance;
  GTlsInputStreamNssPrivate *priv;
};

GType         g_tls_input_stream_nss_get_type (void) G_GNUC_CONST;
GInputStream *g_tls_input_stream_nss_new      (GTlsConnectionNss *conn);

G_END_DECLS

#endif /* __G_TLS_INPUT_STREAM_NSS_H___ */
