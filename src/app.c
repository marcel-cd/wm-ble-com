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
 * @file    app.c
 *
 */

#include "app_app.h"

#include "app_settings.h"
#include "led.h"
#include "button.h"

#define DEBUG_LOG_MODULE_NAME "APP"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

// projcet includes
#include "fsm.h"
#include "ble.h"
#include "otap.h"

/** WM-SDK not using malloc, initiate the app with static instances: */
static Fsm_context m_fsm_context;
static Ble_context m_ble_context;
static app_settings_t m_app_settings;

/**
 * \brief   Callback for button press
 * \param   button_id
 *          Number of button that was pressed
 * \param   event
 *          Always BUTTON_PRESSED here
 *
 * This function is called when a button is pressed down.
 */
static void button_press_func(uint8_t button_id, button_event_e event)
{
    (void) event;

    m_app_settings.is_sink = !m_app_settings.is_sink;
    AppSettings_store(&m_app_settings);

    // reset the device, so that the new firmware can be started
    NVIC_SystemReset();
}

/** WM-SDK Entry function */
void App_init(const app_global_functions_t* functions) {
    LOG_INIT();
    Led_init();
    Button_init();

    /* Retrieve the settings (default values or if available from persistent store) */
    AppSettings_settingsGet(&m_app_settings);


    // check if we have to do an OTAP
    if (m_app_settings.do_otap == 1) {
        LOG(LVL_INFO, "OTAP requested, starting OTAP");
        // copy otap buffer to scratch
        uint32_t ret = Otap_init();
        if (ret == APP_RES_OK) {
          ret = Otap_process();
        }
        if (ret != APP_RES_OK) {
            LOG(LVL_ERROR, "OTAP failed, error code: %d", ret);
        }
        // reset m_app_settings.do_otap to 0
        m_app_settings.do_otap = 0;
        AppSettings_store(&m_app_settings);

        // reset the device, so that the new firmware can be started
        NVIC_SystemReset();
    }

    // Turn LED on if we are a sink
    Led_set(0, m_app_settings.is_sink ? 1 : 0);

    Button_register_for_event(0, BUTTON_PRESSED, button_press_func);

    // create main state machine
    Fsm_createStatic(&m_fsm_context, &m_ble_context, &m_app_settings);
    // firing up main state machine
    Sm_fireEvent(m_fsm_context.sm_context_p, sm_E_INIT, 500);

}
