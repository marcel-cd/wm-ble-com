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

#ifndef APP_APP_H_
#define APP_APP_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// WM-SDK includes
#include "api.h"
#include "app_scheduler.h"
#include "node_configuration.h"
#include "shared_data.h"
#include "stack_state.h"
#include "sl_list.h"
#include "app_persistent.h"

// App includes
#include "__assert.h"
#include "error.h"

#define NUM(a) (sizeof(a) / sizeof(*a))
#define UNUSED(x) (void)(x)
#define LOG_LEVEL 4

typedef struct __attribute__ ((packed)) {
    uint16_t record_magic;

    uint32_t address;

    uint32_t network_address;

    uint8_t network_channel;

    /** this Flag is used after a reboot to trigger OTAP */
    uint8_t do_otap;
    /** this Flag is used to set the node in Sink Mode */
    uint8_t is_sink;
    // non persistent settings
    uint32_t sink_address;
}
app_settings_t;


/** @name State-Machine
 *  @{
 */

/** States */
typedef enum {
    /** state machine will keep current state */
    sm_S_NO_NEW_STATE = 0,
    /** filter will pass given event for all states */
    sm_S_ANY_STATE,
    /** startup state:
     * First line in @ref sm_event_matrix_t should have @ref sm_E_INIT to @ref sm_S_IDLE */
    sm_S_BOOT,
    /** first state after boot @see sm_S_BOOT */
    sm_S_IDLE,

    /** device is waiting for smartphone beacon */
    ble_S_SCANNING,
    /** beacon from smartphone received,
     * send connecting beacons to smartphone */
    ble_S_CONNECTING,
    /** The connection to the Smartphone successfully established
     * Login is not required for this. */
    ble_S_CONNECTED,
} sm_state_e;

/** Events */
typedef enum {
    /** used internally */
    sm_E_NONE = 0,
    /** trigger an error event */
    sm_E_ERROR,
    /** @see sm_S_BOOT  */
    sm_E_INIT,
    /** used to send Module back to @ref sm_S_IDLE  */
    sm_E_IDLE,

    /** Trigger a (delayed) reboot  */
    fsm_E_REBOOT,

    /** trigger @ref sm_M_BLE to listen for smartphone beacons */
    ble_E_SCANNING_START,
    /** trigger @ref sm_M_BLE to send connecting beacons to smarthphone  */
    ble_E_CONNECTING_START,
    /** trigger @ref sm_M_BLE smartphone successfully connected to cModul
     * we have a valid token for this smartphone, which will be checked on each transaction */
    ble_E_CONNECTED,
    /** @ref sm_M_BLE did not receive a advertising from smartphone within given Timeout  */
    ble_E_TIMEOUT,
} sm_event_e;


#define sm_EVENT_QUEUE_LEN_FSM            UINT8_C( 10 )
#define sm_EVENT_QUEUE_LEN_BLE            UINT8_C( 10 )

/** @} */




/** @brief map state to string (used in debugging)
 *
 * @param   sm_state
 *          @ref sm_state_e
 * @return  char
 *          pointer to the string
 */
char* Sm_getStateName(sm_state_e sm_state);

/** @brief map event to string (used in debugging)
 *
 * @param   app_event
 *          @ref app_event_e
 * @return  char
 *          pointer to the string
 */
char* Sm_getEventName(sm_event_e sm_event);

/** @brief map wirepas node role to string (used in debugging)
 *
 * @param   node_role
 *          see app_lib_settings_role_e in wm-sdk/api/wms-settings.h
 * @return  char
 *          pointer to the string
 */
char* Sm_getNodeRoleName(app_lib_settings_role_e node_role);


#endif // APP_APP_H_
