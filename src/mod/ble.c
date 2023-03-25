/* *
 * Wirepas BLE communication example
 *
 * Made in the swiss alps, 2023 <marcel.graber@steinel.ch>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    ble.c
 * @brief   ble (advertising) communication module
 *
 */

#include "app_app.h"
#include "ble.h"
#include "sm.h"
#include "otap.h"

#define DEBUG_LOG_MODULE_NAME "BLE"
#ifdef DEBUG_APP_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_APP_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"

/* ===========================================================================
 * declaration
 * =========================================================================*/

#define EVENT_QUEUE_LEN sm_EVENT_QUEUE_LEN_BLE


/* -------------------------------------------------------------------------*/
/* {{{ local members
 * -------------------------------------------------------------------------*/
/**
 * use modules as singletons -
 * malloc() is not available in WM-SDK, allocate the modules statically
 * child modules will be allocated in fsm.c (or in the unit tests) */
static Sm_context donotuse_sm_context;
// dirty, but otherwise we need to change WM-SDK lib_beacon_rx
static Ble_context * m_ble_context_p;

static sm_event_queue_t m_event_queue[EVENT_QUEUE_LEN];

/** used to ignore already processed packages in beacon_rx callback */
static uint8_t m_lastReceivedPackage[40] = {0};
/** used to copy incoming ble package and free the caller */
static uint8_t m_ble_rx_buffer[40] = {0};


/** @brief add the command to the queue and trigger the task for sending
 *
 *  @param cmd package to send
 *  @param cmd_len length of the command
 *  @returns error code @ref error.h
 *  */
static uint32_t bleSendCmd(Ble_context* context, ble_adv_cmd_t* const cmd, uint8_t cmd_len, bool qos);

/* }}} local memebers */

/* -------------------------------------------------------------------------*/
/* {{{ tasks
 * -------------------------------------------------------------------------*/
/** @name Tasks
 * @{ */

/** @brief task handles the ble_tx data backlog
 *  @param me reference to the local Ble instance */
static uint32_t bleSendTask(void* me);
/** @} name Tasks */
/* }}} tasks */

/* -------------------------------------------------------------------------*/
/* {{{ callbacks / handler
 * -------------------------------------------------------------------------*/

static void bleReceiveCb(const app_lib_beacon_rx_received_t* packet);

/* }}} callbacks / hndler */

/* ----------------------------------------------------------------------------*/
/* {{{ state machine
 * ----------------------------------------------------------------------------*/
/** @name state machine effects and guards
 * @{ */

/** @brief state effect:
 *  - trigger app_E_INIT on all submodules
 *  - change fsm state to @ref fsm_S_SINGLE or fsm_S_MESH
 *  @warning call this effect only once in app lifetime
 *  @param context_p reference to the local context */
static void initialize (void* context_p);
/** @brief state effect
 *  - starts BLE RX periodic or always on (based on power state)
 *  @param context_p reference to the local context */
static void scanningStart(void* context_p);
/** @brief state effect
 *  - stop BLE RX
 *  @param context_p reference to the local context */
static void scanningStop(void* context_p);
/** @brief state effect:
 *  - starts scanner and connection check (timeout)
 *  @param context_p reference to the local context */
static bool isSink(void *context_p) {
    Ble_context* ble_context_p = (Ble_context*)context_p;
    return ble_context_p->app_settings_p->is_sink;
};

/** @brief Definition of State Machine Event Matrix
 */
// *INDENT-OFF*
static const sm_event_matrix_t m_event_matrix[] = {
/* MODULE    Current State         Event guard, next State        EntryAction          Exit Action */
{ sm_S_BOOT, sm_E_INIT,            NULL,        sm_S_IDLE,        initialize,          NULL },
/* Idle */
{ sm_S_IDLE, ble_E_SCANNING_START, isSink,      ble_S_SCANNING,   scanningStart,       scanningStop },
};
// *INDENT-ON*

/** @} name state machine */
/* }}} state machine */

/* ==============================================================================
 * local functions
 * ============================================================================*/

/* ----------------------------------------------------------------------------*/
/* {{{ helper
 * ----------------------------------------------------------------------------*/
/** @brief request new message_id for sending a packet
 * - message ids are valid from 1-16383, see com_adv_cmd_t
 * - in Unit Test mode, the message id will always start by UNIT_TESTS_START_MESSAGE_ID
 *
 * @param[in, out] context current context, will change value message_id
 * @return next free message id
 * */
static uint16_t getNextMessageId(Ble_context* context) {
    if (context == NULL) {
        LOG(LVL_ERROR, "Context not set");
        return 0;
    }

    uint16_t message_id = context->message_id;

    // check if we need to reset the message id
    if (message_id == 0xffff) {
        message_id = 0;
    }

    message_id = message_id + 1;

    // important: set the message id in the context, so we can use it for the next message (before flagging)
    context->message_id = message_id;

    return message_id;
}

static uint32_t getBufferFromCmd(
    ble_adv_cmd_t* const cmd,
    uint8_t* buffer,
    uint8_t buffer_len) {

    memset(buffer, 0, buffer_len);
    memcpy(buffer, (uint8_t*)cmd, buffer_len);
    LOG_BUFFER(LVL_DEBUG, buffer, buffer_len);
    return APP_RES_OK;
}

static ble_adv_cmd_t* getCmdFromBuffer(uint8_t* data, uint32_t data_len) {
    if (data_len > BLE_ADV_TOTAL_LEN) {
        LOG(LVL_DEBUG, "advertising package to big: %d", data_len);
        return NULL;
    } else if (data_len <= BLE_ADV_HEADER_LEN) {
        LOG(LVL_DEBUG, "advertising package to small: %d", data_len);
        return NULL;
    }

    ble_adv_cmd_t* cmd = (ble_adv_cmd_t*)data;
    return cmd;
}


/**
 * @brief   Modify random Bluetooth LE beacon address with device's addresses
 */
static void setBlePrivateStaticAddress(Ble_context* context) {
    app_addr_t node_address;

    node_address = getUniqueAddress();

    // Address sent in the beacon is defined as "static device address" (TxAdd: Random)
    // as defined by the BLE standard
    context->ble_mac_address[0] = 0x13; // set last two bits (mark this address as static private)
    context->ble_mac_address[1] = (uint8_t)(node_address) & 0xff;
    context->ble_mac_address[2] = (uint8_t)(node_address >>  8) & 0xff;
    context->ble_mac_address[3] = (uint8_t)(node_address >>  16) & 0xff;
    context->ble_mac_address[4] = (uint8_t)(node_address >>  24) & 0xff;
    context->ble_mac_address[5] = 0x14; // some random value

    LOG(LVL_INFO, "Ble Address: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x", context->ble_mac_address[5],
        context->ble_mac_address[4], context->ble_mac_address[3], context->ble_mac_address[2], context->ble_mac_address[1],
        context->ble_mac_address[0]);
}

static uint16_t generateBleTxHeader(Ble_context* context, uint8_t* buffer, uint16_t buffer_len, uint8_t ad_type,
                                    uint8_t payload_len) {
    if (buffer_len < sizeof(ble_tx_header_t)) {
        LOG(LVL_ERROR, "buffer to small");
        return 0;
    }

    ble_tx_header_t common_frame = {
        .ad_type = BLE_HEADER_PDU_TYPE,
        .nid[0] = context->ble_mac_address[0],
        .nid[1] = context->ble_mac_address[1],
        .nid[2] = context->ble_mac_address[2],
        .nid[3] = context->ble_mac_address[3],
        .nid[4] = context->ble_mac_address[4],
        .nid[5] = context->ble_mac_address[5],
        .ad_data_len = payload_len + 3, // add 3-Bytes  for ad_type and ad_data_len
        .ad_data_type = ad_type,
        .company_id = BLE_COMPANY_ID
        /* .ad_flags_data = 0x04,      /\* Bluetooth LE Beacon only *\/ */
    };
    memcpy(buffer, &common_frame, sizeof(common_frame));
    return sizeof(common_frame);
}

static uint32_t bleSendCmd(Ble_context* context, ble_adv_cmd_t* const cmd, uint8_t cmd_len, bool qos) {
    __ASSERT(NULL != cmd, "cmd must not be NULL");
    __ASSERT(cmd_len <= CMD_ADV_TOTAL_LEN, "cmd size is longer than allowed");
    __ASSERT(cmd_len > CMD_ADV_HEADER_LEN, "cmd does not have a payload");


    for (uint16_t i = 0; i < BLE_TX_LIST_LEN; i++) {
        // find free slot
        if (context->ble_tx_items[i].status == ble_SLOT_EMPTY) {
            context->ble_tx_items[i].status = ble_SLOT_SEND;
            context->ble_tx_items[i].payload_len = cmd_len;
            context->ble_tx_items[i].qos = qos;
            context->ble_tx_items[i].message_id = cmd->message_id;
            getBufferFromCmd(
                cmd,
                context->ble_tx_items[i].payload,
                cmd_len );
            lib_system->enterCriticalSection();
            sl_list_push_back(&context->ble_tx_items_head_p, (sl_list_t*)&context->ble_tx_items[i]);
            lib_system->exitCriticalSection();
            break;
        }
    }

    if (App_Scheduler_addTask_execTime_Caller(bleSendTask, context,
            APP_SCHEDULER_SCHEDULE_ASAP,
            10) != APP_SCHEDULER_RES_OK) {
        LOG(LVL_ERROR, "Cannot start task to send ble data");
        return APP_RET_ERROR_INTERNAL;
    }

    return APP_RET_OK;
}

static void configureLibBeaconTx() {

    lib_beacon_tx->clearBeacons();
    lib_beacon_tx->setBeaconInterval(100); // from 100ms to 60 seconds
    int8_t power = 8;                      // 8 dBm
    lib_beacon_tx->setBeaconPower(0, &power);
    lib_beacon_tx->setBeaconChannels(0, APP_LIB_BEACON_TX_CHANNELS_ALL); // All channels
    /* lib_beacon_tx->setBeaconChannels(0, APP_LIB_BEACON_TX_CHANNELS_38); */
}

/* }}} helper */


/* ----------------------------------------------------------------------------*/
/* {{{ state machine effects / guards
 * ----------------------------------------------------------------------------*/

static void initialize(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    Ble_context* ble_context_p = (Ble_context*) context_p;

    if (ble_context_p->initialized) {
        LOG(LVL_ERROR, "initialize calld again");
        return;
    }

    /* handle received ble packages */
    lib_beacon_rx->setBeaconReceivedCb(bleReceiveCb);
    setBlePrivateStaticAddress(m_ble_context_p);

    ble_context_p->initialized = true;
    LOG(LVL_DEBUG, "initialize() done");
}

/* uint8_t buffer[32]; */
static void scanningStart(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    Ble_context* ble_context_p = (Ble_context*) context_p;

    // Start scanner
    app_res_e res = lib_beacon_rx->startScanner(APP_LIB_BEACON_RX_CHANNEL_ALL);
    if (res != APP_RES_OK) {
        LOG(LVL_ERROR, "Cannot start scanner");
    }

    // reset connected device mac
    memset(ble_context_p->connected_device_mac, 0, 6);

    LOG(LVL_DEBUG, "scanningStart() done");
}
static void scanningStop(void* context_p) {
    UNUSED(context_p);

    app_res_e res = lib_beacon_rx->stopScanner();
    if (res != APP_RES_OK) {
        LOG(LVL_ERROR, "Cannot start scanner");
    }

    LOG(LVL_DEBUG, "scanningStop() done");
}
/* }}} state machine effects / guards */

/* ----------------------------------------------------------------------------*/
/* {{{ callbacks / handler
 * ----------------------------------------------------------------------------*/


/** @brief this callback will be called from lib_beacon_rx, when a new package arrives
 *
 * @param packet the format in packet->payload includes the mac-address in front (6 bytes)
 * */
static void bleReceiveCb(const app_lib_beacon_rx_received_t* packet) {
    __ASSERT(m_ble_context_p->com_context_p != NULL, "need Com module for CRC");
    uint8_t buffer_len = 0;

    // prefilter the data, to reduce load
    //
    // Android package, ad_data_type = 0xFF and the company id for steinel solutions has to be set
    //
    // IOS package, the data has fixed length  mac(6 Byte) / ad_data_len(1 Byte) / ad_data_type (1 Byte) / UUID (16 Byte)
    // and ad_data_len = 16  and ad_data_type = 0x07
    if (packet->length > 10 && packet->payload[7] == BLE_ADV_DATA_TYPE_MANUFACTURER &&
            packet->payload[8] == ((uint8_t)(BLE_COMPANY_ID) & 0xFF)
            && packet->payload[9] == ((uint8_t)(BLE_COMPANY_ID >> 8) & 0xFF)
       ) {
        // this package is interesting, keep on going
        // set buffer to correct address
        uint8_t offset = + 6 + 2 + 2; // mac address (6) ad_data_len (1) ad_data_type(1) company_id (2)
        memcpy(m_ble_rx_buffer, packet->payload + offset, packet->length - offset);
        buffer_len = packet->length - offset; // same here
    } else if (packet->length == 30 && packet->payload[13] == BLE_ADV_DATA_TYPE_SERVICE_UUID) {
        /* LOG(LVL_INFO, "IOS package: %d", packet->length); */
        /* LOG_BUFFER(LVL_INFO, packet->payload, packet->length); */
        uint8_t offset = + 6 + 2 +
                         6; // mac address (6) + ad_data_len (1) ad_data_type(1) + ios sends two flags (2x3 Bytes)?? */

        for (uint8_t i = 0; i < 16; i++) {
            m_ble_rx_buffer[i] = packet->payload[packet->length - 1 - i];
        }

        /* buffer = packet->payload + offset; */
        buffer_len = packet->length - offset;
        /* buffer_len = 6; */
    } else {
        return;
    }

    // nothing to do, if com_context is not set
    ble_adv_cmd_t* cmd_rx = getCmdFromBuffer(m_ble_rx_buffer, buffer_len);

    // the package does not have a vliad crc
    if (cmd_rx == NULL) {
        return;
    }

    // check if we have the same package again -> do this after the CRC Check as we dont compare unwanted packages
    // buffer is decrypted, because we modify the reference in getCmdFromBuffer
    if (memcmp(m_lastReceivedPackage, m_ble_rx_buffer, buffer_len) == 0) {
        // same package, ignore
        return;
    }

    // set this package as last received
    memcpy(m_lastReceivedPackage, m_ble_rx_buffer, buffer_len);

    // check if the address matches
    if ( m_ble_context_p->last_received_message_id == cmd_rx->message_id &&
            cmd_rx->message_id > 1) {
        // same package, ignore
        return;
    }
    m_ble_context_p->last_received_message_id = cmd_rx->message_id;

    if (cmd_rx->command == (ble_ADV_CMD_SCAN_REQUEST)) {
        // for simplicity just use the Nordic Unique ID
        m_ble_context_p->connected_token = (getUniqueAddress() & 0xFFFF);
        LOG(LVL_INFO, "Scan request Msg: %d, token: %d", cmd_rx->message_id, m_ble_context_p->connected_token);
        ble_adv_cmd_t cmd_rsp = {
            .message_id =  getNextMessageId(m_ble_context_p),
            .command = ble_ADV_CMD_SCAN_RESPONSE,
            .payload.scan_rsp.request_id = cmd_rx->message_id,
            .payload.scan_rsp.token = m_ble_context_p->connected_token,
            .payload.scan_rsp.firmware_version_major = VER_MAJOR,
            .payload.scan_rsp.firmware_version_minor = VER_MINOR,
            .payload.scan_rsp.is_sink = m_ble_context_p->app_settings_p->is_sink
        };

        bleSendCmd(m_ble_context_p, &cmd_rsp, BLE_ADV_CMD_SCAN_RSP_LEN, 0);
        Sm_fireEvent(m_ble_context_p->sm_context_p, ble_E_CONNECTING_START, 500);
        return;

    } else if (cmd_rx->command == (ble_ADV_CMD_OTAP_BEGIN_UPLOAD_REQUEST)) {
        LOG(LVL_INFO, "OTAP Begin Upload Msg: %d", cmd_rx->message_id);
        m_ble_context_p->otap.adv_package_length = cmd_rx->payload.otap_begin_upload_req.package_length;
        m_ble_context_p->otap.scratchpad_length = cmd_rx->payload.otap_begin_upload_req.scratchpad_length;
        m_ble_context_p->otap.scratchpad_seqeunce_number = cmd_rx->payload.otap_begin_upload_req.scratchpad_sequence_number;
        // use fileupload partition (message_id 0x8000 - 0xFFFF)
        m_ble_context_p->otap.start_message_id = 0x8000;
        m_ble_context_p->otap.end_message_id = m_ble_context_p->otap.start_message_id +
                                               m_ble_context_p->otap.scratchpad_length / m_ble_context_p->otap.adv_package_length +
                                               (m_ble_context_p->otap.scratchpad_length % m_ble_context_p->otap.adv_package_length ? 1 : 0) - 1;
        m_ble_context_p->otap.total_messages = m_ble_context_p->otap.end_message_id - m_ble_context_p->otap.start_message_id + 1;
        // set received message flags
        memset(m_ble_context_p->otap.messageReceived,0, sizeof(m_ble_context_p->otap.messageReceived));

        // check if settings are ok
        int ret = Otap_init();

        if (ret != APP_RET_OK) {
            LOG(LVL_ERROR, "Otap_init failed: %d", ret);
        }

        ret = Otap_bufferBegin();

        if (ret != APP_RET_OK) {
            LOG(LVL_ERROR, "Buffer_init failed: %d", ret);
        }

        // give feedback to the app, that we are ready to receive the data
        ble_adv_cmd_t cmd_rsp = {
            .message_id =  getNextMessageId(m_ble_context_p),
            .command = ble_ADV_CMD_OTAP_BEGIN_UPLOAD_RESPONSE,
            .payload.otap_begin_upload_rsp.request_id = cmd_rx->message_id,
            .payload.otap_begin_upload_rsp.start_message_id = m_ble_context_p->otap.start_message_id,
            .payload.otap_begin_upload_rsp.response_code = ret,
        };
        bleSendCmd(m_ble_context_p, &cmd_rsp, BLE_ADV_CMD_OTAP_BEGIN_UPLOAD_RSP_LEN, 0);
    } else if (cmd_rx->command == (ble_ADV_CMD_OTAP_UPLOAD_REQUEST) &&
               m_ble_context_p->otap.start_message_id <= cmd_rx->message_id &&
               m_ble_context_p->otap.end_message_id >= cmd_rx->message_id) {
        int message_id = cmd_rx->message_id - m_ble_context_p->otap.start_message_id;
        // write the data to the buffer
        int ret = Otap_bufferWrite(&cmd_rx->payload.otap_upload_req.data_start, m_ble_context_p->otap.adv_package_length,
                                   (cmd_rx->message_id - m_ble_context_p->otap.start_message_id) * m_ble_context_p->otap.adv_package_length);

        if (ret != APP_RET_OK) {
            LOG(LVL_ERROR, "otap_upload failed: %d", ret);
        } else {
            // set the message received flag
            m_ble_context_p->otap.messageReceived[message_id/8] |= 1 << (message_id % 8);
        }
        bool lastMessageReceived = m_ble_context_p->otap.messageReceived[(m_ble_context_p->otap.total_messages-1) / 8] & (1 << ((m_ble_context_p->otap.total_messages-1) % 8));
        // be kind, and send some status messages back:
        if (cmd_rx->message_id % 10 == 0 || lastMessageReceived) {
            int percentage = (int)(message_id * 90 / m_ble_context_p->otap.total_messages);
            if (lastMessageReceived) {
                int missing_messages = 0;
                for (int i = 0; i < m_ble_context_p->otap.total_messages; i++) {
                    if (!(m_ble_context_p->otap.messageReceived[i / 8] &
                            (1 << (i % 8)))) {
                        missing_messages++;
                    }
                }
                percentage = 90 + (int)(10 / (missing_messages + 1));
            }
            LOG(LVL_INFO, "OTAP Upload Status Msg: %d/%d", message_id,
                m_ble_context_p->otap.total_messages);
            ble_adv_cmd_t cmd_rsp = {
                .message_id = getNextMessageId(m_ble_context_p),
                .command = ble_ADV_CMD_OTAP_UPLOAD_RESPONSE,
                .payload.otap_upload_rsp.request_id = cmd_rx->message_id,
                .payload.otap_upload_rsp.response_code = ble_STATUS_OTAP_UPLOAD,
                .payload.otap_upload_rsp.percentage = percentage,
            };
            // wait, till we have answer to this message:
            bleSendCmd(m_ble_context_p, &cmd_rsp,
                       BLE_ADV_CMD_OTAP_UPLOAD_RSP_LEN, 0);
        }


        // check if we have received the last message
        if (lastMessageReceived) {
            LOG(LVL_INFO, "otap_upload finished");
            // do we have all messages?
            for (int i = 0; i < m_ble_context_p->otap.total_messages; i++) {
                if (!(m_ble_context_p->otap.messageReceived[i/8] & (1 << (i % 8)))) {
                    LOG(LVL_ERROR, "OTAP_UPLOAD_REQUEST Msg: missing message: %d", i);
                    // we have a missing message, send a request for it
                    ble_adv_cmd_t cmd_req = {
                        .message_id =  getNextMessageId(m_ble_context_p),
                        .command = ble_ADV_CMD_RESEND_MESSAGE_REQUEST,
                        .payload.resend_message_req.resend_message_id = m_ble_context_p->otap.start_message_id + i
                    };
                    m_ble_context_p->keep_sending = cmd_req.message_id;
                    bleSendCmd(m_ble_context_p, &cmd_req, BLE_ADV_CMD_RESEND_MESSAGE_REQ_LEN, 1);
                    return;
                }
            }

            // upload is done
            ret = Otap_bufferEnd(
                      m_ble_context_p->otap.scratchpad_length,
                      m_ble_context_p->otap.scratchpad_seqeunce_number);

            if (ret != APP_RET_OK) {
                LOG(LVL_ERROR, "otap_upload failed: %d", ret);
            }

            ble_adv_cmd_t cmd_rsp = {
                .message_id = getNextMessageId(m_ble_context_p),
                .command = ble_ADV_CMD_OTAP_UPLOAD_RESPONSE,
                .payload.otap_upload_rsp.request_id = cmd_rx->message_id,
                .payload.otap_upload_rsp.response_code = ble_STATUS_OTAP_OK,
                .payload.otap_upload_rsp.percentage = 100
            };
            m_ble_context_p->keep_sending = 0;
            bleSendCmd(m_ble_context_p, &cmd_rsp,
                       BLE_ADV_CMD_OTAP_UPLOAD_RSP_LEN, 0);
            // set flag: in next reboot, process OTAP Image
            m_ble_context_p->app_settings_p->do_otap = 1;
            AppSettings_store(m_ble_context_p->app_settings_p);

            // send a reboot command
            Sm_fireEvent(m_ble_context_p->fsm_sm_context_p, fsm_E_REBOOT, 500);
        }
    }
}
/* }}} callbacks / handler */

/* ----------------------------------------------------------------------------*/
/* {{{ tasks
 * ----------------------------------------------------------------------------*/


static uint32_t bleSendTask(void* me) {
    __ASSERT(me != NULL, "caller not set");
    __ASSERT(((Ble_context*)me)->app_settings_p != NULL, "missing app context");

    // keep this static
    static volatile ble_tx_list_item_t* sending_element = NULL;
    static volatile bool beacon_sending = false;
    ble_tx_list_item_t* element;
    Ble_context* ble = (Ble_context*)me;
    uint8_t buffer[BLE_ADV_TOTAL_LEN + sizeof(ble_tx_header_t)] = {0};

    // lock, if we have to wait for important messages
    if (sending_element && sending_element->qos && ble->keep_sending == sending_element->message_id) {
        LOG(LVL_INFO, "waiting for important message: %d", sending_element->message_id);
        // keep sending the beacon
        return 500;
    }

    if (sending_element) {
        // free this slot:
        sending_element->status = ble_SLOT_EMPTY;
        sending_element = NULL;
    }

    lib_system->enterCriticalSection();
    uint8_t list_size = sl_list_size(&ble->ble_tx_items_head_p);
    lib_system->exitCriticalSection();

    /**  2. Check, if we are in sending progress
     */
    if (list_size == 0) {
        // did we send a beacon?

        // no more data to send
        // stop the task still next ble_send_data calls this task
        if (lib_beacon_tx->enableBeacons(false) != APP_RES_OK) {
            // Cannot stop, try again in 500ms
            return 500;
        }

        // mark, that in the next call we have to enable the beacon_tx
        beacon_sending = false;
        return APP_SCHEDULER_STOP_TASK;
    }


    /** 3. Get the data from the buffer and send it
     */
    lib_system->enterCriticalSection();
    element = (ble_tx_list_item_t*) sl_list_pop_front(&ble->ble_tx_items_head_p);
    lib_system->exitCriticalSection();

    // Add the header (with the Mac Address) and the Manufacturer AD Type
    uint16_t header_len = generateBleTxHeader(ble, buffer, sizeof(buffer), BLE_ADV_DATA_TYPE_MANUFACTURER,
                          element->payload_len );

    if (header_len == 0) {
        LOG(LVL_ERROR, "something wrong with the header");
        // break in case of failure:
        return APP_SCHEDULER_RES_NO_MORE_TASK;
    }

    memcpy(buffer + header_len, element->payload, element->payload_len);

    if (beacon_sending == false) {
        configureLibBeaconTx();
        int res = lib_beacon_tx->enableBeacons(true);

        if (res != APP_RES_OK) {
            LOG(LVL_ERROR, "failure in sending beacon");

            // cannot enabel beacon, try again in 200ms
            // add the item back to the list, as we already removed it
            lib_system->enterCriticalSection();
            sl_list_push_front(&ble->ble_tx_items_head_p, (sl_list_t*) element);
            lib_system->exitCriticalSection();
            return 250;
        }
    }

    lib_beacon_tx->setBeaconContents(0, buffer, header_len + element->payload_len);
    LOG_BUFFER(LVL_DEBUG, element->payload,  element->payload_len);
    LOG(LVL_DEBUG, "beacon-tx payload: %d", element->payload_len);

    // everything fine, ble_beacon shoud send data:
    sending_element = element;
    sending_element->status = ble_SLOT_SENDING;

    // give long delay for first beacon
    if (beacon_sending == false) {
        beacon_sending = true;
        LOG(LVL_DEBUG, "first beacon sent");
        return 2000;
    }

    return 250; // 100ms
}

/* }}} tasks */

/* ==============================================================================
 *  global functions
 * ============================================================================*/

/* ----------------------------------------------------------------------------*/
/* {{{ init
 * ----------------------------------------------------------------------------*/
extern void Ble_createStatic(Ble_context* const ble_context_p,
                             app_settings_t* app_settings_p) {
    __ASSERT(NULL != ble_context_p, "ble_context_p must not be NULL");
    __ASSERT(NULL != app_settings_p, "app_settings_p must not be NULL");

    m_ble_context_p = ble_context_p;
    m_ble_context_p->sm_context_p = &donotuse_sm_context;
    m_ble_context_p->app_settings_p = app_settings_p;

    m_ble_context_p->initialized = false;
    m_ble_context_p->connected_token = 0;

    // init the queue
    sl_list_init(&m_ble_context_p->ble_tx_items_head_p);

    /* create the local fsm state machine */
    Sm_createStatic(m_ble_context_p->sm_context_p, ble_context_p,
                    DEBUG_LOG_MODULE_NAME, m_event_matrix, NUM(m_event_matrix),
                    m_event_queue, EVENT_QUEUE_LEN);
}
extern void Ble_destroyStatic(Ble_context* const me) {
    // not much to do here in static variant
    Sm_destroyStatic(me->sm_context_p);
}

/* }}} init */
