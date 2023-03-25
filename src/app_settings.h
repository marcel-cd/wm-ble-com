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
 * @file    app_settings.h
 * @brief   Header for the application settings source.
 *
 */
#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

/* global include, WM-SDK uses app.h, so be creative and use the following name */
#include "app_app.h"

/** Magic id definition in Applib persistent data
*   !!! DO NOT change
*/
#define APPLIB_SETTINGS_RECORD_MAGIC                 0x2020



/**
 * @brief   Configure the node network parameters.
 *          Node settings are either the default or persistent
 *          storage (when available)
 * @param   settings
 *          Pointer to a applib_settings_t structure where settings
 *          will be written
 *
 * @return  bool
 *          true: node configured,  false: error
 */
bool AppSettings_configureNode(app_settings_t * settings);

/**
 * @brief   Provides the Applib specific settings
 *
 * @param   settings
 *          Pointer to a applib_settings_t structure where settings
 *          will be written
 */
void AppSettings_settingsGet(app_settings_t * settings);

/**
 * @brief   Stores the Applib specific settings tp persistent
 *
 * @param   settings
 *          Pointer to a applib_settings_t structure containing the settings
 *          will be written
 * @return  bool
 *          true: settings saved, false: settings not saved
 *          due to: error | persistent not supported | no change
 */
bool AppSettings_store(app_settings_t * settings);


#endif
