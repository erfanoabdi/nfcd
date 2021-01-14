/*
 * Copyright (C) 2020-2021 Jolla Ltd.
 * Copyright (C) 2020-2021 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NFC_INITIATOR_H
#define NFC_INITIATOR_H

#include "nfc_types.h"

#include <glib-object.h>

G_BEGIN_DECLS

/* Since 1.1.0 */

typedef struct nfc_initiator_priv NfcInitiatorPriv;

struct nfc_initiator {
    GObject object;
    NfcInitiatorPriv* priv;
    NFC_TECHNOLOGY technology;
    NFC_PROTOCOL protocol;
    /* This one-way flag is set to FALSE when peer goes away. */
    gboolean present;
};

GType nfc_initiator_get_type(void) NFCD_EXPORT;
#define NFC_TYPE_INITIATOR (nfc_initiator_get_type())
#define NFC_INITIATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
        NFC_TYPE_INITIATOR, NfcInitiator))

NfcInitiator*
nfc_initiator_ref(
    NfcInitiator* initiator)
    NFCD_EXPORT;

void
nfc_initiator_unref(
    NfcInitiator* initiator)
    NFCD_EXPORT;

G_END_DECLS

#endif /* NFC_INITIATOR_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
