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
 * @file    ble.h
 * @brief   Header for the BLE Module
 *
 */
#ifndef BLE_H_
#define BLE_H_

#include "app_settings.h"
#include "sm.h"

/** @addtogroup BLE
 * @{
 */

/** maximal backlog for sending BLE advertising packages */
#define BLE_TX_LIST_LEN 1024

/* 4096 packages * 12 bytes = 49152 bytes max */
#define BLE_OTAP_MAX_NUMBER_OF_PACKAGES 4096

/** used in header */
#define BLE_HEADER_PDU_TYPE 0x42                  // Non-connectable Beacon
#define BLE_ADV_DATA_TYPE_MANUFACTURER 0xFF       // Manufacturer Data with varaible length
#define BLE_ADV_DATA_TYPE_SERVICE_UUID 0x07       // Complete 128-Bit Service ID
#define BLE_COMPANY_ID 0x09EF                     // Steinel Solutions AG

/** we can send up to 28 Bytes as payload
 *
 * 37 Bytes - Address(6 Bytes) - AD-Length(1 Byte) - AD-Type(1 Byte) - CompanyID(2 Byte)
 **/
#define BLE_ADV_TOTAL_LEN (27)
/** every cmd has the following header (3 Bytes)
 * (CRC not needed, bacause ble advertising packages itself are CRC protected)
 * \par
 * - bool file_transfer : 1;
 * - uint16_t message_id : 15;
 * - com_adv_cmd_type_e command_type : 1;
 * - com_adv_cmd_e command : 7;
 */
#define BLE_ADV_HEADER_LEN  3
/** total lenth of the payload (23), reduced by the BLE_HEADER_LEN  */
#define BLE_ADV_PAYLOAD_LEN (BLE_ADV_TOTAL_LEN - BLE_ADV_HEADER_LEN)

/**
 *  Request or rsponse?
 */
typedef enum {
    /** command is a request */
    ble_ADV_CMD_TYPE_REQUEST = 0x00,
    /** command is a response to a request */
    ble_ADV_CMD_TYPE_RESPONSE = 0x80,
} ble_adv_cmd_type_e;

/**
 *  Command codes in advertising packages
 */
typedef enum {
    ble_ADV_CMD_RESEND_MESSAGE_REQUEST   = 0x01,
    ble_ADV_CMD_RESEND_MESSAGE_RESPONSE  = (ble_ADV_CMD_RESEND_MESSAGE_REQUEST | ble_ADV_CMD_TYPE_RESPONSE),

    ble_ADV_CMD_SCAN_REQUEST     = 0x02,
    ble_ADV_CMD_SCAN_RESPONSE    = (ble_ADV_CMD_SCAN_REQUEST | ble_ADV_CMD_TYPE_RESPONSE),

    ble_ADV_CMD_OTAP_BEGIN_UPLOAD_REQUEST   = 0x0A,
    ble_ADV_CMD_OTAP_BEGIN_UPLOAD_RESPONSE  = (ble_ADV_CMD_OTAP_BEGIN_UPLOAD_REQUEST | ble_ADV_CMD_TYPE_RESPONSE),

    ble_ADV_CMD_OTAP_UPLOAD_REQUEST   = 0x0B,
    ble_ADV_CMD_OTAP_UPLOAD_RESPONSE  = (ble_ADV_CMD_OTAP_UPLOAD_REQUEST | ble_ADV_CMD_TYPE_RESPONSE),
} ble_adv_cmd_e;

/**
 *    - [0:1]  message_id
 */
typedef struct __attribute((packed)) {
    uint16_t resend_message_id;
}
ble_adv_cmd_resend_message_req_t;
#define BLE_ADV_CMD_RESEND_MESSAGE_REQ_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_resend_message_req_t))

/**
 *    - [0:1] requestId
 */
typedef struct __attribute((packed)) {
    uint16_t resend_message_id;
}
ble_adv_cmd_resend_message_rsp_t;
#define BLE_ADV_CMD_RESEND_MESSAGE_RSP_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_resend_rsp_t))

/**
 *  - request (from app):
 *    - [0] app version
 *    - [1] hardware os (0: android / 1: ios)
 */
typedef struct __attribute((packed)) {
    uint8_t app_version;
    uint8_t hardware;
}
ble_adv_cmd_scan_req_t;
#define BLE_ADV_CMD_SCAN_REQ_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_scan_req_t))

/**
 *    - [0:1] requestId
 *    - [2:3]  Token 2 Bytes
 *    - [4]    firmware version major
 *    - [5]    firmware version minor
 *    - [6]    is sink (0: no / 1: yes)
 */
typedef struct __attribute((packed)) {
    uint16_t request_id;
    uint16_t token;
    uint8_t  firmware_version_major;
    uint8_t  firmware_version_minor;
    uint8_t  is_sink;
}
ble_adv_cmd_scan_rsp_t;
#define BLE_ADV_CMD_SCAN_RSP_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_scan_rsp_t))

/**
 *  - request (from app):
 *    - [0:1]  token
 *    - [2]    scratchpad sequnce
 *    - [3:6]  scratchpad length
 *    - [7]    package length (every package must have same length, so we can avoid sending length information in each package, IOS will send 12 Bytes, Android 23 Bytes)
 */
typedef struct __attribute((packed)) {
    uint16_t token;
    uint8_t  scratchpad_sequence_number;
    uint32_t scratchpad_length;
    uint8_t  package_length;
}
ble_adv_cmd_otap_begin_upload_req_t;
#define BLE_ADV_CMD_OTAP_BEGIN_UPLOAD_REQ_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_otap_begin_upload_req_t))

typedef enum {
    ble_STATUS_OTAP_OK = 0,
    ble_STATUS_OTAP_UPLOAD = 1,
    ble_STATUS_OTAP_ERR_OVERLOAD = 2,
} ble_status_otap_e;

/**
 *    - [0:1] requestId
 *    - [2] response code (0-> success)
 */
typedef struct __attribute((packed)) {
    uint16_t request_id;
    uint16_t start_message_id;
    uint8_t  response_code;
}
ble_adv_cmd_otap_begin_upload_rsp_t;
#define BLE_ADV_CMD_OTAP_BEGIN_UPLOAD_RSP_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_otap_begin_upload_rsp_t))

/**
 *  - request (from app):
 *    - [0:x]  data
 */
typedef struct __attribute((packed)) {
    uint8_t data_start;
}
ble_adv_cmd_otap_upload_req_t;

/**
 *    - [0:1] requestId
 *    - [2] response code (0-> success)
 */
typedef struct __attribute((packed)) {
    uint16_t request_id;
    uint8_t  response_code;
    uint8_t  percentage;
}
ble_adv_cmd_otap_upload_rsp_t;
#define BLE_ADV_CMD_OTAP_UPLOAD_RSP_LEN (BLE_ADV_HEADER_LEN + sizeof(ble_adv_cmd_otap_upload_rsp_t))

typedef struct  __attribute__ ((packed)) {
    /** this includes the encrypted and last flag */
    uint16_t message_id;
    uint8_t command;
    /** Union of the various payload structures for all serial packets. */
    union __attribute((packed)) {
        ble_adv_cmd_resend_message_req_t  resend_message_req;
        ble_adv_cmd_resend_message_rsp_t  resend_message_rsp;
        ble_adv_cmd_scan_req_t  scan_req;
        ble_adv_cmd_scan_rsp_t  scan_rsp;
        ble_adv_cmd_otap_begin_upload_req_t otap_begin_upload_req;
        ble_adv_cmd_otap_begin_upload_rsp_t otap_begin_upload_rsp;
        ble_adv_cmd_otap_upload_req_t otap_upload_req;
        ble_adv_cmd_otap_upload_rsp_t otap_upload_rsp;
    }
    payload;
}
ble_adv_cmd_t;

/**
 * @brief Common beacon header
 */
typedef struct __attribute((packed)) {
    uint8_t ad_type;
    uint8_t nid[6];
    /** has to be set in every TX task */
    uint8_t ad_data_len;
    /** use 0xFF (Manufacturer) Advertising Type */
    uint8_t ad_data_type;
    uint16_t company_id;
}
ble_tx_header_t;

/**
 * @brief Common beacon header used in Unit Tests
 */
typedef struct __attribute((packed)) {
    uint8_t nid[6];
    /** has to be set in every TX task */
    uint8_t ad_data_len;
    /** use 0xFF (Manufacturer) Advertising Type or 0x07 (ServiceID) */
    uint8_t ad_data_type;
    uint16_t company_id;
}
ble_rx_header_manufacturer_t;

/**
 * @brief Common beacon header used in Unit Tests
 */
typedef struct __attribute((packed)) {
    uint8_t nid[6];
    /** has to be set in every TX task */
    uint8_t ad_data_len;
    /** use 0xFF (Manufacturer) Advertising Type or 0x07 (ServiceID) */
    uint8_t ad_data_type;
}
ble_rx_header_service_t;

typedef enum {
    ble_OTAP_STATE_IDLE = 0,
    ble_OTAP_STATE_UPLOAD = 1,
    ble_OTAP_STATE_FAILED = 2
} ble_otap_state_t;

/**
 * @brief OTAP transfer state
 */
typedef struct {
    uint8_t scratchpad_seqeunce_number;
    uint32_t scratchpad_length;
    uint8_t adv_package_length;
    uint16_t start_message_id;
    uint16_t end_message_id;
    uint16_t total_messages;
    uint8_t messageReceived[BLE_OTAP_MAX_NUMBER_OF_PACKAGES/8];
    ble_otap_state_t state;
}
ble_otap_t;

typedef struct Ble Ble_context;

typedef enum {
    ble_SLOT_EMPTY = 0,
    ble_SLOT_SEND = 1,
    ble_SLOT_SENDING = 2,
    ble_SLOT_SENDING_CONFIRMED = 3
} ble_slot_t;

typedef struct {
    /**  needed by sl_list in wm-sdk */
    sl_list_t list; // Reserved for sl_list use
    /** used to mark this slot as free */
    ble_slot_t status;
    /** hold data to send
     * @warning statically allocated, do not overflow */
    uint8_t payload[BLE_ADV_TOTAL_LEN];
    uint8_t payload_len;
    /* set this to true, if we need an answer, before sendning the next message */
    bool qos;
    uint16_t message_id;
} ble_tx_list_item_t;


/** @brief Instance to the local "Class"
 * includes all settings, which needs to be set from the unit tests
 * to inject the right startup behaviour and isolate the unit tests
 * to small units
 */
struct Ble {
    /** this flag is used to guard the call to initialize() twice */
    bool initialized;
    /** pointer to the global app_settings_t
     * every modul will use the same */
    app_settings_t* app_settings_p;
    /** Every modul has its own state machine, just use a static allocation here */
    Sm_context* sm_context_p;
    /** Firing Events to the Main Statemachine */
    Sm_context* fsm_sm_context_p;
    /** needed in sl_list to push and pop list items */
    sl_list_head_t ble_tx_items_head_p;
    /** array with the backlog for sending data */
    ble_tx_list_item_t ble_tx_items[BLE_TX_LIST_LEN];

    /** used to store the current state of the OTAP transfer */
    ble_otap_t otap;

    /** used as a counter to close a connection, if no data since xxx ms */
    uint16_t last_received_message_id;
    /** last sent message id */
    uint16_t message_id;

    /** used in otap process to wait for incoming messages */
    volatile uint16_t keep_sending;

    /** Ble Private Static address based on the  */
    uint8_t ble_mac_address[6];  // Storage for Node Id and Network address

    /** Connected Smartphone BLE Mac, will be used for identification */
    uint8_t connected_device_mac[6];
    /** timestamp in seconds since last ping from Device received
     * used for timeout check in closing connection
     * if this value is -1, we do not have a connected device */
    int64_t connected_device_last_ping_s;
    /** generated after Scan Request, and used to filter incoming messages */
    uint16_t connected_token;

};


/** @brief Create BLE main state machine
 *
 * Use this function to start the main state machine without malloc
 *
 * @warning This function requires that:
 *               - app_settings_p is not NULL
 *               - ble_context_p is not NULL
 *               - com_context_p is not NULL, if lib_beacon_rx callback is used
 *
 * @param[in, out] ble_context_p Pointer to the instance
 * @param[in, out] app_settings_p Pointer to the global application settings
 */
extern void Ble_createStatic(Ble_context* const ble_context_p,
    app_settings_t* app_settings_p);


/** @brief Destroy BLE main state machine
 *
 * Use this function to cleanup the static allocated instance
 *
 * @param[in, out] ble_context_p Pointer to the instance
 */
extern void Ble_destroyStatic(Ble_context* const ble_context_p);

/** @} addtogroup BLE */
#endif // BLE_H_
