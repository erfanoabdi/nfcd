/*
 * Copyright (C) 2018-2021 Jolla Ltd.
 * Copyright (C) 2018-2021 Slava Monich <slava.monich@jolla.com>
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

#include "nfc_adapter_p.h"
#include "nfc_adapter_impl.h"
#include "nfc_peer_services.h"
#include "nfc_initiator_impl.h"
#include "nfc_initiator_p.h"
#include "nfc_target_impl.h"
#include "nfc_tag_t2.h"
#include "nfc_peer.h"

#include "test_common.h"
#include "test_target.h"
#include "test_initiator.h"

#include <gutil_log.h>

static TestOpt test_opt;

static const guint8 symm_data[] = { 0x00, 0x00 };

static
void
test_adapter_inc(
    NfcAdapter* adapter,
    void* user_data)
{
    (*(int*)user_data)++;
}

static
void
test_adapter_tag_inc(
    NfcAdapter* adapter,
    NfcTag* tag,
    void* user_data)
{
    (*(int*)user_data)++;
}

static
void
test_adapter_peer_inc(
    NfcAdapter* adapter,
    NfcPeer* peer,
    void* user_data)
{
    (*(int*)user_data)++;
}

/*==========================================================================*
 * Test adapter
 *==========================================================================*/

typedef NfcAdapterClass TestAdapterClass;
typedef struct test_adapter {
    NfcAdapter adapter;
    gboolean fail_power_request;
    gboolean power_request_pending;
    gboolean power_requested;
    gboolean fail_mode_request;
    gboolean mode_request_pending;
    NFC_MODE mode_requested;
} TestAdapter;

G_DEFINE_TYPE(TestAdapter, test_adapter, NFC_TYPE_ADAPTER)
#define TEST_TYPE_ADAPTER (test_adapter_get_type())
#define TEST_ADAPTER(obj) (G_TYPE_CHECK_INSTANCE_CAST(obj, \
        TEST_TYPE_ADAPTER, TestAdapter))

TestAdapter*
test_adapter_new(
    void)
{
    return g_object_new(TEST_TYPE_ADAPTER, NULL);
}

static
void
test_adapter_complete_power_request(
    TestAdapter* self)
{
    g_assert(self->power_request_pending);
    self->power_request_pending = FALSE;
    nfc_adapter_power_notify(&self->adapter, self->power_requested, TRUE);
}

static
void
test_adapter_fail_power_request(
    TestAdapter* self)
{
    g_assert(self->power_request_pending);
    self->power_request_pending = FALSE;
    nfc_adapter_power_notify(&self->adapter, !self->power_requested, TRUE);
}

static
void
test_adapter_complete_mode_request(
    TestAdapter* self)
{
    g_assert(self->mode_request_pending);
    self->mode_request_pending = FALSE;
    nfc_adapter_mode_notify(&self->adapter, self->mode_requested, TRUE);
}

static
void
test_adapter_fail_mode_request(
    TestAdapter* self,
    NFC_MODE mode)
{
    g_assert(self->mode_request_pending);
    self->mode_request_pending = FALSE;
    nfc_adapter_mode_notify(&self->adapter, mode, TRUE);
}

static
gboolean
test_adapter_submit_power_request(
    NfcAdapter* adapter,
    gboolean on)
{
    TestAdapter* self = TEST_ADAPTER(adapter);

    g_assert(!self->power_request_pending);
    if (self->fail_power_request) {
        return NFC_ADAPTER_CLASS(test_adapter_parent_class)->
            submit_power_request(adapter, on);
    } else {
        self->power_requested = on;
        self->power_request_pending = TRUE;
        return TRUE;
    }
}

static
void
test_adapter_cancel_power_request(
    NfcAdapter* adapter)
{
    TestAdapter* self = TEST_ADAPTER(adapter);

    g_assert(self->power_request_pending);
    self->power_request_pending = FALSE;
    NFC_ADAPTER_CLASS(test_adapter_parent_class)->cancel_power_request(adapter);
}

static
gboolean
test_adapter_submit_mode_request(
    NfcAdapter* adapter,
    NFC_MODE mode)
{
    TestAdapter* self = TEST_ADAPTER(adapter);

    g_assert(!self->mode_request_pending);
    if (self->fail_mode_request) {
        return NFC_ADAPTER_CLASS(test_adapter_parent_class)->
            submit_mode_request(adapter, mode);
    } else {
        self->mode_requested = mode;
        self->mode_request_pending = TRUE;
        return TRUE;
    }
}

static
void
test_adapter_cancel_mode_request(
    NfcAdapter* adapter)
{
    TestAdapter* self = TEST_ADAPTER(adapter);

    g_assert(self->mode_request_pending);
    self->mode_request_pending = FALSE;
    NFC_ADAPTER_CLASS(test_adapter_parent_class)->cancel_mode_request(adapter);
}

static
void
test_adapter_init(
    TestAdapter* self)
{
}

static
void
test_adapter_class_init(
    NfcAdapterClass* klass)
{
    klass->submit_power_request = test_adapter_submit_power_request;
    klass->cancel_power_request = test_adapter_cancel_power_request;
    klass->submit_mode_request = test_adapter_submit_mode_request;
    klass->cancel_mode_request = test_adapter_cancel_mode_request;
}

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    /* Public interfaces are NULL tolerant */
    g_assert(!nfc_adapter_ref(NULL));
    g_assert(!nfc_adapter_peers(NULL));
    g_assert(!nfc_adapter_request_mode(NULL, 0));
    g_assert(!nfc_adapter_add_tag_t2(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_t4a(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_t4b(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_initiator_a(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_initiator_f(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_target_a(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_target_f(NULL, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_target_presence_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_added_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_removed_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_added_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_removed_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_powered_changed_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_power_requested_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_mode_changed_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_mode_requested_handler(NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_enabled_changed_handler(NULL, NULL, NULL));
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    g_assert(!nfc_adapter_add_other_tag(NULL, NULL));
    G_GNUC_END_IGNORE_DEPRECATIONS

    nfc_adapter_set_name(NULL, NULL);
    nfc_adapter_set_services(NULL, NULL);
    nfc_adapter_mode_notify(NULL, 0, FALSE);
    nfc_adapter_target_notify(NULL, FALSE);
    nfc_adapter_power_notify(NULL, FALSE, FALSE);
    nfc_adapter_set_enabled(NULL, TRUE);
    nfc_adapter_request_power(NULL, TRUE);
    nfc_adapter_remove_tag(NULL, NULL);
    nfc_adapter_remove_peer(NULL, NULL);
    nfc_adapter_remove_handler(NULL, 0);
    nfc_adapter_remove_handlers(NULL, NULL, 0);
    nfc_adapter_unref(NULL);
}

/*==========================================================================*
 * basic
 *==========================================================================*/

static
void
test_basic(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcAdapter* adapter = &test->adapter;
    NfcPeerServices* services = nfc_peer_services_new();
    const char* name = "test";

    g_assert(!nfc_adapter_add_target_presence_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_added_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_removed_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_powered_changed_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_power_requested_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_mode_changed_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_mode_requested_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_enabled_changed_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_added_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_removed_handler(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_t2(adapter, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_t4a(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_tag_t4b(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_initiator_a(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_initiator_f(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_target_a(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_target_f(adapter, NULL, NULL, NULL));
    g_assert(!nfc_adapter_add_other_tag2(adapter, NULL, NULL));
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    g_assert(!nfc_adapter_add_other_tag(adapter, NULL));
    G_GNUC_END_IGNORE_DEPRECATIONS
    nfc_adapter_remove_handler(adapter, 0);

    nfc_adapter_set_name(adapter, name);
    nfc_adapter_set_services(adapter, services);
    g_assert(!g_strcmp0(adapter->name, name));

    g_assert(nfc_adapter_ref(adapter) == adapter);
    nfc_adapter_unref(adapter);
    nfc_adapter_unref(adapter);
    nfc_peer_services_unref(services);
}

/*==========================================================================*
 * enabled
 *==========================================================================*/

static
void
test_enabled(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcAdapter* adapter = &test->adapter;
    int enabled_changed_count = 0;
    gulong enabled_id = nfc_adapter_add_enabled_changed_handler(adapter,
        test_adapter_inc, &enabled_changed_count);

    nfc_adapter_set_name(adapter, "test");
    
    nfc_adapter_set_enabled(adapter, TRUE);
    g_assert(enabled_changed_count == 1);

    /* Second time it has no effect */
    nfc_adapter_set_enabled(adapter, TRUE);
    g_assert(enabled_changed_count == 1);

    nfc_adapter_set_enabled(adapter, FALSE);
    g_assert(enabled_changed_count == 2);

    nfc_adapter_remove_handler(adapter, enabled_id);
    nfc_adapter_unref(adapter);
}

/*==========================================================================*
 * power
 *==========================================================================*/

static
void
test_power(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcAdapter* adapter = &test->adapter;
    int power_requested_count = 0, powered_changed_count = 0;
    gulong id[2];

    nfc_adapter_set_name(adapter, "test");
    
    id[0] = nfc_adapter_add_power_requested_handler(adapter,
        test_adapter_inc, &power_requested_count);
    id[1] = nfc_adapter_add_powered_changed_handler(adapter,
        test_adapter_inc, &powered_changed_count);

    test->fail_power_request = TRUE;
    g_assert(!adapter->power_requested);
    nfc_adapter_request_power(adapter, TRUE);
    g_assert(adapter->power_requested);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    /* Second time it has no effect */
    nfc_adapter_request_power(adapter, TRUE);
    g_assert(!power_requested_count);

    /* No request is actually submitted because it's not enabled */
    g_assert(!test->power_request_pending);

    /* This tries to submit power request to the implementation but fails */
    nfc_adapter_set_enabled(adapter, TRUE);
    g_assert(!test->power_request_pending);

    /* Disable/enable the adapter to give it another try */
    test->fail_power_request = FALSE;
    nfc_adapter_set_enabled(adapter, FALSE);
    nfc_adapter_set_enabled(adapter, TRUE);
    g_assert(test->power_request_pending);

    /* Cancel power-on (and fail the power-off request) */
    test->fail_power_request = TRUE;
    nfc_adapter_request_power(adapter, FALSE);
    test->fail_power_request = FALSE;
    g_assert(!test->power_request_pending);
    g_assert(!powered_changed_count);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    /* Fail power-on */
    nfc_adapter_request_power(adapter, TRUE);
    g_assert(power_requested_count == 1);
    nfc_adapter_power_notify(adapter, FALSE, FALSE); /* Ignored */
    test_adapter_fail_power_request(test);
    g_assert(power_requested_count == 2);
    g_assert(!test->power_request_pending);
    g_assert(!adapter->power_requested);
    g_assert(!powered_changed_count);
    power_requested_count = 0;

    /* Simulate successful power-on */
    nfc_adapter_request_power(adapter, TRUE);
    g_assert(adapter->power_requested);
    g_assert(!adapter->powered);
    g_assert(test->power_request_pending);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    test_adapter_complete_power_request(test);
    g_assert(adapter->powered);
    g_assert(powered_changed_count == 1);
    powered_changed_count = 0;

    /* Unsolicited power changes */
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    g_assert(!powered_changed_count);
    nfc_adapter_power_notify(adapter, FALSE, FALSE);
    g_assert(powered_changed_count == 1);
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    g_assert(!power_requested_count);
    g_assert(powered_changed_count == 2);
    powered_changed_count = 0;

    /* Power-off with active mode change request pending */
    adapter->supported_modes = NFC_MODE_READER_WRITER;
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_READER_WRITER));
    g_assert(test->mode_request_pending);

    nfc_adapter_request_power(adapter, FALSE);
    g_assert(!test->mode_request_pending); /* Canceled */
    g_assert(adapter->powered);
    g_assert(!powered_changed_count);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    test_adapter_complete_power_request(test);
    g_assert(!adapter->powered);
    g_assert(!power_requested_count);
    g_assert(powered_changed_count == 1);
    powered_changed_count = 0;

    /* Cancel power-on in progress */
    nfc_adapter_request_power(adapter, TRUE);
    g_assert(!powered_changed_count);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    nfc_adapter_request_power(adapter, FALSE);
    g_assert(!powered_changed_count);
    g_assert(power_requested_count == 1);
    power_requested_count = 0;

    /* Disable won't do anything (power-off is already pending */
    nfc_adapter_set_enabled(adapter, FALSE);
    g_assert(test->power_request_pending);

    /* nfc_adapter_dispose will cancel the last power request */
    nfc_adapter_remove_all_handlers(adapter, id);
    nfc_adapter_unref(adapter);
}

/*==========================================================================*
 * mode
 *==========================================================================*/

static
void
test_mode(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcAdapter* adapter = &test->adapter;
    int mode_requested_count = 0, mode_changed_count = 0;
    gulong id[2];

    nfc_adapter_set_name(adapter, "test");
    
    id[0] = nfc_adapter_add_mode_requested_handler(adapter,
        test_adapter_inc, &mode_requested_count);
    id[1] = nfc_adapter_add_mode_changed_handler(adapter,
        test_adapter_inc, &mode_changed_count);

    /* Unsupported mode */
    g_assert(!nfc_adapter_request_mode(adapter, NFC_MODE_READER_WRITER));
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_NONE));
    g_assert(!mode_requested_count);
    g_assert(!mode_changed_count);

    /* Successful switch to NFC_MODE_READER_WRITER */
    adapter->supported_modes = NFC_MODE_READER_WRITER | NFC_MODE_P2P_INITIATOR;
    g_assert(!nfc_adapter_request_mode(adapter, NFC_MODE_CARD_EMILATION));
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_READER_WRITER |
        NFC_MODE_CARD_EMILATION));
    g_assert(!mode_changed_count);
    g_assert(mode_requested_count == 1);
    mode_requested_count = 0;

    g_assert(!test->mode_request_pending); /* No power yet */
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    g_assert(test->mode_request_pending);

    test->fail_mode_request = TRUE; /* This one will fail: */
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_P2P_INITIATOR));
    g_assert(mode_requested_count == 1);
    test->fail_mode_request = FALSE; /* And this one succeed: */
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_READER_WRITER));
    g_assert(mode_requested_count == 2);
    mode_requested_count = 0;

    test_adapter_complete_mode_request(test);
    g_assert(adapter->mode == NFC_MODE_READER_WRITER);
    g_assert(!mode_requested_count);
    g_assert(mode_changed_count == 1);
    mode_changed_count = 0;

    /* Spontaneous mode changes */
    nfc_adapter_mode_notify(adapter, NFC_MODE_READER_WRITER, FALSE);
    g_assert(!mode_changed_count);
    nfc_adapter_mode_notify(adapter, NFC_MODE_NONE, FALSE);
    g_assert(mode_changed_count == 1);
    nfc_adapter_mode_notify(adapter, NFC_MODE_READER_WRITER, FALSE);
    g_assert(mode_changed_count == 2);
    g_assert(!mode_requested_count);
    mode_changed_count = 0;

    /* Fail to switch polling off */
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_NONE));
    g_assert(mode_requested_count == 1);
    test_adapter_fail_mode_request(test, NFC_MODE_READER_WRITER);
    g_assert(!mode_changed_count);
    g_assert(mode_requested_count == 2);
    mode_requested_count = 0;

    /* Switching power off will switch polling off too */
    nfc_adapter_power_notify(adapter, FALSE, FALSE);
    g_assert(adapter->mode == NFC_MODE_NONE);
    g_assert(!mode_requested_count);
    g_assert(mode_changed_count == 1);
    mode_changed_count = 0;

    /* Switching power back on will (try to) switch polling on */
    test->fail_mode_request = TRUE;
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    test->fail_mode_request = FALSE;
    g_assert(!mode_requested_count);
    g_assert(!mode_changed_count);
    g_assert(!test->mode_request_pending);

    /* Toggle power again to give it another try */
    nfc_adapter_power_notify(adapter, FALSE, FALSE);
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    g_assert(test->mode_request_pending);

    /* But switch power off before mode change is completed */
    nfc_adapter_power_notify(adapter, FALSE, FALSE);
    g_assert(!mode_requested_count);
    g_assert(!mode_changed_count);
    g_assert(!test->mode_request_pending);

    /* This time it's going to work */
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    g_assert(test->mode_request_pending);
    test_adapter_complete_mode_request(test);
    g_assert(adapter->mode == NFC_MODE_READER_WRITER);
    g_assert(!mode_requested_count);
    g_assert(mode_changed_count == 1);
    mode_changed_count = 0;

    /* Switch it off and back on */
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_NONE));
    g_assert(mode_requested_count == 1);
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_READER_WRITER));
    g_assert(test->mode_request_pending);
    g_assert(mode_requested_count == 2);
    g_assert(!mode_changed_count);
    mode_requested_count = 0;

    test_adapter_complete_mode_request(test);
    g_assert(adapter->mode == NFC_MODE_READER_WRITER);
    g_assert(!mode_requested_count);
    g_assert(!mode_changed_count);

    /* nfc_adapter_dispose will cancel the last mode request */
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_NONE));
    nfc_adapter_mode_notify(adapter, NFC_MODE_NONE, FALSE); /* Ignored */
    nfc_adapter_remove_all_handlers(adapter, id);
    nfc_adapter_unref(adapter);
}

/*==========================================================================*
 * tags
 *==========================================================================*/

static
void
test_tags(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcTarget* target0 = test_target_new_tech(NFC_TECHNOLOGY_A, FALSE);
    NfcTarget* target1 = test_target_new(FALSE);
    NfcAdapter* adapter = &test->adapter;
    NfcTag* tag0;
    NfcTag* tag1;
    gulong id[3];
    int tag_added = 0, tag_removed = 0, presence_changed_count = 0;
    NfcParamPoll poll;

    id[0] = nfc_adapter_add_tag_added_handler(adapter,
        test_adapter_tag_inc, &tag_added);
    id[1] = nfc_adapter_add_tag_removed_handler(adapter,
        test_adapter_tag_inc, &tag_removed);
    id[2] = nfc_adapter_add_target_presence_handler(adapter,
        test_adapter_inc, &presence_changed_count);
    g_assert(id[0]);
    g_assert(id[1]);
    g_assert(id[2]);

    /* Set up the adapter */
    nfc_adapter_set_name(adapter, "test");
    adapter->supported_modes = NFC_MODE_READER_WRITER;
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    nfc_adapter_mode_notify(adapter, NFC_MODE_READER_WRITER, FALSE);
    
    /* Test "presence_changed" signal */
    nfc_adapter_target_notify(adapter, TRUE);
    g_assert(!adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,0);

    memset(&poll, 0, sizeof(poll));
    tag0 = nfc_adapter_add_tag_t2(adapter, target0, &poll.a);
    g_assert(adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,1);

    nfc_adapter_target_notify(adapter, TRUE); /* Has no effect */
    g_assert_cmpint(presence_changed_count, == ,1);

    tag1 = nfc_adapter_add_other_tag2(adapter, target1, NULL);
    g_assert(tag0);
    g_assert(tag1);
    g_assert(!g_strcmp0(tag0->name, "tag0"));
    g_assert(!g_strcmp0(tag1->name, "tag1"));
    g_assert(tag_added == 2);
    g_assert(tag_removed == 0);

    /* Target will remain present until last tag is removed */
    nfc_adapter_target_notify(adapter, FALSE);
    g_assert(nfc_adapter_request_mode(adapter, NFC_MODE_NONE));
    g_assert(adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,1);

    /* Remove the tags */
    nfc_target_gone(target0);
    nfc_adapter_remove_tag(adapter, tag1->name);
    g_assert(!adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,2);
    g_assert_cmpint(tag_added, == ,2);
    g_assert_cmpint(tag_removed, == ,2);

    /* These have no effect */
    nfc_adapter_remove_tag(adapter, NULL);
    nfc_adapter_remove_tag(adapter, "foo");

    /* This target is no longer present: */
    g_assert(!nfc_adapter_add_other_tag2(adapter, target0, &poll));
    g_assert(tag_removed == 2);

    nfc_adapter_remove_all_handlers(adapter, id);
    nfc_adapter_unref(adapter);
    nfc_target_unref(target0);
    nfc_target_unref(target1);
}

/*==========================================================================*
 * peer
 *==========================================================================*/

static
void
test_peer(
    void)
{
    static const TestTx tx[] = {
        {
            { TEST_ARRAY_AND_SIZE(symm_data) },
            { TEST_ARRAY_AND_SIZE(symm_data) }
        },{
            { TEST_ARRAY_AND_SIZE(symm_data) },
            { TEST_ARRAY_AND_SIZE(symm_data) }
        }
    };
    static const guint8 general_bytes [] = {
        0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02,
        0x07, 0xff, 0x03, 0x02, 0x00, 0x13, 0x04, 0x01,
        0xff
    };
    static const NfcParamNfcDepInitiator initiator_param = {
        { TEST_ARRAY_AND_SIZE(general_bytes) }
    };

    TestAdapter* test = test_adapter_new();
    NfcTarget* target0 = test_target_new_with_tx(TEST_ARRAY_AND_COUNT(tx));
    NfcTarget* target1 = test_target_new_with_tx(TEST_ARRAY_AND_COUNT(tx));
    NfcAdapter* adapter = &test->adapter;
    NfcPeer* peer0;
    NfcPeer* peer1;
    int peer_added = 0, peer_removed = 0, presence_changed_count = 0;
    gulong id[3];

    /* Set up the adapter */
    nfc_adapter_set_name(adapter, "test");
    adapter->supported_modes = NFC_MODE_P2P_TARGET;
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    nfc_adapter_mode_notify(adapter, NFC_MODE_P2P_TARGET, FALSE);
    g_assert(!adapter->target_present);

    id[0] = nfc_adapter_add_peer_added_handler(adapter,
        test_adapter_peer_inc, &peer_added);
    id[1] = nfc_adapter_add_peer_removed_handler(adapter,
        test_adapter_peer_inc, &peer_removed);
    id[2] = nfc_adapter_add_target_presence_handler(adapter,
        test_adapter_inc, &presence_changed_count);
    g_assert(id[0]);
    g_assert(id[1]);
    g_assert(id[2]);

    /* Two peers are unlikely in real life but API allows it  */
    peer0 = nfc_adapter_add_peer_initiator_a(adapter, target0, NULL,
        &initiator_param);
    g_assert(peer0);
    g_assert(adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,1);
    g_assert_cmpint(peer_added, == ,1);
    g_assert_cmpint(peer_removed, == ,0);

    peer1 = nfc_adapter_add_peer_initiator_a(adapter, target1, NULL,
        &initiator_param);
    g_assert(peer1);
    g_assert(adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,1);
    g_assert_cmpint(peer_added, == ,2);
    g_assert_cmpint(peer_removed, == ,0);

    /* These two have no effect */
    nfc_adapter_remove_peer(adapter, NULL);
    nfc_adapter_remove_peer(adapter, "");
    g_assert(adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,1);
    g_assert_cmpint(peer_added, == ,2);
    g_assert_cmpint(peer_removed, == ,0);

    /* This one does */
    nfc_peer_ref(peer0);
    nfc_adapter_remove_peer(adapter, peer0->name);
    g_assert(adapter->target_present); /* One is still present */
    g_assert_cmpint(presence_changed_count, == ,1);
    g_assert_cmpint(peer_added, == ,2);
    g_assert_cmpint(peer_removed, == ,1);
    nfc_peer_unref(peer0);

    /* The second one goes away by itself */
    nfc_target_gone(target1);
    g_assert(!adapter->target_present); /* Both are gone now */
    g_assert_cmpint(presence_changed_count, == ,2);
    g_assert_cmpint(peer_added, == ,2);
    g_assert_cmpint(peer_removed, == ,2);

    /* Fail to add a non-present peer */
    g_assert(!nfc_adapter_add_peer_initiator_a(adapter, target1, NULL,
        &initiator_param));

    nfc_adapter_remove_all_handlers(adapter, id);
    nfc_adapter_unref(adapter);
    nfc_target_unref(target0);
    nfc_target_unref(target1);
}

/*==========================================================================*
 * no_peer
 *==========================================================================*/

static
void
test_no_peer(
    void)
{
    TestAdapter* test = test_adapter_new();
    NfcTarget* target = test_target_new(FALSE);
    NfcInitiator* initiator = test_initiator_new();
    NfcAdapter* adapter = &test->adapter;
    int peer_added = 0, peer_removed = 0, presence_changed_count = 0;
    gulong id[3];

    /* Set up the adapter */
    nfc_adapter_set_name(adapter, "test");
    adapter->supported_modes = NFC_MODE_P2P_TARGET;
    nfc_adapter_power_notify(adapter, TRUE, FALSE);
    nfc_adapter_mode_notify(adapter, NFC_MODE_P2P_TARGET, FALSE);
    g_assert(!adapter->target_present);

    id[0] = nfc_adapter_add_peer_added_handler(adapter,
        test_adapter_peer_inc, &peer_added);
    id[1] = nfc_adapter_add_peer_removed_handler(adapter,
        test_adapter_peer_inc, &peer_removed);
    id[2] = nfc_adapter_add_target_presence_handler(adapter,
        test_adapter_inc, &presence_changed_count);
    g_assert(id[0]);
    g_assert(id[1]);
    g_assert(id[2]);

    /* Try to add a peer (and fail) */
    g_assert(!nfc_adapter_add_peer_initiator_a(adapter, target, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_initiator_f(adapter, target, NULL, NULL));
    g_assert(!adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,0);
    g_assert_cmpint(peer_added, == ,0);
    g_assert_cmpint(peer_removed, == ,0);

    g_assert(!nfc_adapter_add_peer_target_a(adapter, initiator, NULL, NULL));
    g_assert(!nfc_adapter_add_peer_target_f(adapter, initiator, NULL, NULL));
    g_assert(!adapter->target_present);
    g_assert_cmpint(presence_changed_count, == ,0);
    g_assert_cmpint(peer_added, == ,0);
    g_assert_cmpint(peer_removed, == ,0);

    nfc_adapter_remove_all_handlers(adapter, id);
    nfc_adapter_unref(adapter);
    g_object_unref(initiator);
    nfc_target_unref(target);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_(name) "/core/adapter/" name

int main(int argc, char* argv[])
{
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    g_type_init();
    G_GNUC_END_IGNORE_DEPRECATIONS;
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("enabled"), test_enabled);
    g_test_add_func(TEST_("power"), test_power);
    g_test_add_func(TEST_("mode"), test_mode);
    g_test_add_func(TEST_("tags"), test_tags);
    g_test_add_func(TEST_("peer"), test_peer);
    g_test_add_func(TEST_("no_peer"), test_no_peer);
    test_init(&test_opt, argc, argv);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
