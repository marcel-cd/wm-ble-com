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

#include "app_settings.h"
#define DEBUG_LOG_MODULE_NAME "SETTINGS"
#ifdef DEBUG_APP_LOG_MAX_LEVEL
    #define DEBUG_LOG_MAX_LEVEL DEBUG_APP_LOG_MAX_LEVEL
#else
    #define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"

/*
 *  Network keys define in mcu/common/start.c and
 *  used only if default_network_cipher_key and default_network_authen_key
 *  are defined in one of the config.mk (set to NULL otherwise)
 */
extern const uint8_t* authen_key_p;
extern const uint8_t* cipher_key_p;


static void get_default_settings(app_settings_t* settings) {
    /* Note all defined fields must be set to their default value */

    memset(settings, 0x00, sizeof(app_settings_t));

    settings->record_magic = APPLIB_SETTINGS_RECORD_MAGIC;
    settings->address = getUniqueAddress();
    settings->network_address = CONF_NETWORK_ADDRESS;
    settings->network_channel = CONF_NETWORK_CHANNEL;

    /* General settings */
    settings->do_otap = 0;
    settings->is_sink = 0;
    // speedup reboot during development
    #if DEVELOPMENT_MODE
    settings->is_sink = (getUniqueAddress() == 3073986309);
    #endif
}




bool AppSettings_store(app_settings_t* settings) {
    app_settings_t app_persistent_old;

    if (App_Persistent_read((uint8_t*)&app_persistent_old, sizeof(app_settings_t)) == APP_PERSISTENT_RES_OK) {
        if (memcmp(&app_persistent_old, settings, sizeof(app_settings_t)) == 0) {
            LOG(LVL_INFO, "Settings not updated, skip flash write");
            return false;
        }
    }

    /* Save settings if: different than previous | not yet saved | previous corrupted */
    if (App_Persistent_write((uint8_t*)settings, sizeof(app_settings_t)) == APP_PERSISTENT_RES_OK) {
        LOG(LVL_INFO, "Applib settings written to flash");
    } else {
        LOG(LVL_ERROR, "Applib settings flash write failed");
    }

    return true;
}



bool AppSettings_configureNode(app_settings_t* settings) {
    bool ret = true;

    LOG(LVL_INFO, "network_address: %u %u", settings->network_address ,settings->network_channel);


    app_lib_settings_role_e node_role = APP_LIB_SETTINGS_ROLE_AUTOROLE_LL;
    if (settings->is_sink) {
        node_role = APP_LIB_SETTINGS_ROLE_SINK_LL;
    }

    if (lib_settings->setNodeRole(node_role) != APP_RES_OK) {
        LOG(LVL_ERROR, "Cannot set node role to: %u, node_role");
        ret = false;
    }

    /* Configuration will be applied only for the  parameters which
    are not yet set */
    if (configureNode(settings->address, settings->network_address, settings->network_channel,
            authen_key_p, cipher_key_p) != APP_RES_OK) {
        LOG(LVL_ERROR, "Cannot set node parameters");
        ret = false;
    }

    /* lib_settings->setAcRange(2000, 2000); // Fix access cycle to 8s */


    return ret;
}

void AppSettings_settingsGet(app_settings_t* settings) {
    App_Persistent_read((uint8_t *)settings, sizeof(app_settings_t));
    if (settings->record_magic != APPLIB_SETTINGS_RECORD_MAGIC) {
        LOG(LVL_INFO, "Settings not found in flash, use default settings");
        get_default_settings(settings);
    }
}
