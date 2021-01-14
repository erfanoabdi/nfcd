/*
 * Copyright (C) 2020 Jolla Ltd.
 * Copyright (C) 2020 Slava Monich <slava.monich@jolla.com>
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

#ifndef NFC_PEER_PRIVATE_H
#define NFC_PEER_PRIVATE_H

#include "nfc_types_p.h"
#include "nfc_peer.h"

typedef struct nfc_peer_class {
    GObjectClass object;
    void (*deactivate)(NfcPeer* peer);
    void (*gone)(NfcPeer* peer);
} NfcPeerClass;

#define NFC_PEER_CLASS(klass) G_TYPE_CHECK_CLASS_CAST((klass), \
        NFC_TYPE_PEER, NfcPeerClass)

NfcPeer*
nfc_peer_new_initiator(
    NfcTarget* target,
    NFC_TECHNOLOGY technology,
    const NfcParamNfcDepInitiator* nfc_dep,
    NfcPeerServices* services)
    NFCD_INTERNAL;

NfcPeer*
nfc_peer_new_target(
    NfcInitiator* initiator,
    NFC_TECHNOLOGY technology,
    const NfcParamNfcDepTarget* nfc_dep,
    NfcPeerServices* services)
    NFCD_INTERNAL;

void
nfc_peer_set_name(
    NfcPeer* peer,
    const char* name)
    NFCD_INTERNAL;

/* For use by derived classes */

gboolean
nfc_peer_init_base(
    NfcPeer* peer,
    NfcLlcIo* llc_io,
    const GUtilData* gb, /* ATR_RES/ATR_REQ General Bytes */
    NfcPeerServices* services,
    NFC_TECHNOLOGY technology,
    NFC_PEER_FLAGS flags)
    NFCD_INTERNAL;

void
nfc_peer_gone(
    NfcPeer* peer)
    NFCD_INTERNAL;

#endif /* NFC_PEER_PRIVATE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
