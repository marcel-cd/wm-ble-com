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

/* include global header first */
#include "app_app.h"


char* Sm_getStateName(sm_state_e sm_state) {
    switch (sm_state) {
    case sm_S_NO_NEW_STATE:
        return "sm_S_NO_NEW_STATE";

    case sm_S_ANY_STATE:
        return "sm_S_ANY_STATE";

    case sm_S_BOOT:
        return "sm_S_BOOT";

    case sm_S_IDLE:
        return "sm_S_IDLE";

    case ble_S_SCANNING:
        return "ble_S_SCANNING";

    case ble_S_CONNECTING:
        return "ble_S_CONNECTING";

    case ble_S_CONNECTED:
        return "ble_S_CONNECTED";
    }
    return "-- state not known --";
}

char* Sm_getEventName(sm_event_e sm_event) {
    switch (sm_event) {
    case sm_E_NONE:
        return "sm_E_NONE";

    case sm_E_ERROR:
        return "sm_E_ERROR";

    case sm_E_INIT:
        return "sm_E_INIT";

    case sm_E_IDLE:
        return "sm_E_IDLE";

    case fsm_E_REBOOT:
        return "fsm_E_REBOOT";

    case ble_E_SCANNING_START:
        return "ble_E_SCANNING_START";

    case ble_E_CONNECTING_START:
        return "ble_E_CONNECTING_START";

    case ble_E_CONNECTED:
        return "ble_E_CONNECTED";

    case ble_E_TIMEOUT:
        return "ble_E_TIMEOUT";

    }


    return "-- event not known --";
}

char* Sm_getNodeRoleName(app_lib_settings_role_e node_role) {
    switch (node_role) {
    case APP_LIB_SETTINGS_ROLE_SINK_LE:
        return "APP_LIB_SETTINGS_ROLE_SINK_LE";

    case APP_LIB_SETTINGS_ROLE_SINK_LL:
        return "APP_LIB_SETTINGS_ROLE_SINK_LL";

    case APP_LIB_SETTINGS_ROLE_HEADNODE_LE:
        return "APP_LIB_SETTINGS_ROLE_HEADNODE_LE";

    case APP_LIB_SETTINGS_ROLE_HEADNODE_LL:
        return "APP_LIB_SETTINGS_ROLE_HEADNODE_LL";

    case APP_LIB_SETTINGS_ROLE_SUBNODE_LE:
        return "APP_LIB_SETTINGS_ROLE_SUBNODE_LE";

    case APP_LIB_SETTINGS_ROLE_SUBNODE_LL:
        return "APP_LIB_SETTINGS_ROLE_SUBNODE_LL";

    case APP_LIB_SETTINGS_ROLE_AUTOROLE_LE:
        return "APP_LIB_SETTINGS_ROLE_AUTOROLE_LE";

    case APP_LIB_SETTINGS_ROLE_AUTOROLE_LL:
        return "APP_LIB_SETTINGS_ROLE_AUTOROLE_LL";

    case APP_LIB_SETTINGS_ROLE_ADVERTISER:
        return "APP_LIB_SETTINGS_ROLE_ADVERTISER";
    }

    return "-- role not known --";
}
