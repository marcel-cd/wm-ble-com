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
 * @file    fsm.h
 * @brief   Main state machine
 *
 * @ref app.c needs to call @ref Fsm_createStatic()
 *
 */
#ifndef FSM_H_
#define FSM_H_

#include "app_settings.h"
#include "sm.h"
#include "ble.h"

/** @addtogroup FSM
 * @{
 */
typedef struct Fsm Fsm_context;



/** @brief Instance to the local Fsm "Class"
 * includes all settings, which needs to be set from the unit tests
 * to inject the right startup behaviour and isolate the unit tests
 * to small units
 */
struct Fsm {
  /** this flag is used to guard the call to initialize() twice */
  bool initialized;
  /** pointer to the global app_settings_t
   * every modul will use the same */
  app_settings_t *app_settings_p;
  /** Point to the local state_machine */
  Sm_context *sm_context_p;
  /** pointer to the BLE Module app_M_BLE
   * @warning can be null, eg. when the modul will be deactivated
   *          one need to check this on every access
   * */
  Ble_context *ble_context_p;
};


/** @brief Create FSM main state machine
 *
 * Create main module (state machine) and all child modules
 * (if pointer is not null)
 *
 * @warning This function requires that:
 *               - app_settings_p is not NULL
 *               - fsm_context_p is not NULL
 *
 * @param[in, out] fsm_context_p Pointer to the instance
 * @param[in, out] ble_context_p Pointer to the BLE module
 * @param[in, out] app_settings_p Global persistent settings
 */
extern void Fsm_createStatic(Fsm_context *const fsm_context_p,
                             Ble_context *const ble_context_p,
                             app_settings_t *app_settings_p);

/** @brief Destroy FSM main state machine
 *
 * Use this function to cleanup the static allocated instance
 *
 * @param[in, out] fsm_context_p Pointer to the instance, will be set to {0}
 */
extern void Fsm_destroyStatic(Fsm_context *const fsm_context_p);

/** @} addtogroup FSM */
#endif // FSM_H_
