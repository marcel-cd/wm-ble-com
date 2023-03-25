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
 * @file    error.h
 * @brief   global error codes, also used as error codes in communication with smartphone
 *
 */

/*Header guard */
#ifndef ERROR_H__
#define ERROR_H__


#ifdef __cplusplus
extern "C" {
#endif

#define ERROR_BASE_NUM           (0x0)       ///< Global error base
#define ERROR_BASE_BLE_NUM       (0x40)      ///< Error base for BLE events
#define ERROR_BASE_CRYPTO_NUM    (0x50)      ///< Error base for Crypto events
#define ERROR_BASE_COM_NUM       (0x60)      ///< Error base for Crypto events

#define APP_RET_OK                          (ERROR_BASE_NUM + 0)  ///< Successful command
#define APP_RET_ERROR_INTERNAL              (ERROR_BASE_NUM + 1)  ///< Internal Error
#define APP_RET_NO_MEM                      (ERROR_BASE_NUM + 2)  ///< No Memory for operation
#define APP_RET_NOT_FOUND                   (ERROR_BASE_NUM + 3)  ///< Not found
#define APP_RET_NOT_SUPPORTED               (ERROR_BASE_NUM + 4)  ///< Not supported
#define APP_RET_INVALID_PARAM               (ERROR_BASE_NUM + 5)  ///< Invalid Parameter
#define APP_RET_INVALID_STATE               (ERROR_BASE_NUM + 6)  ///< Invalid state, operation disallowed in this state
#define APP_RET_INVALID_LENGTH              (ERROR_BASE_NUM + 6)  ///< Invalid Length
#define APP_RET_INVALID_FLAGS               (ERROR_BASE_NUM + 8) ///< Invalid Flags
#define APP_RET_INVALID_DATA                (ERROR_BASE_NUM + 9) ///< Invalid Data
#define APP_RET_DATA_SIZE                   (ERROR_BASE_NUM + 10) ///< Invalid Data size
#define APP_RET_TIMEOUT                     (ERROR_BASE_NUM + 11) ///< Operation timed out
#define APP_RET_ERROR_NULL                  (ERROR_BASE_NUM + 12) ///< Null Pointer
#define APP_RET_FORBIDDEN                   (ERROR_BASE_NUM + 13) ///< Forbidden Operation
#define APP_RET_INVALID_ADDR                (ERROR_BASE_NUM + 14) ///< Bad Memory Address
#define APP_RET_BUSY                        (ERROR_BASE_NUM + 15) ///< Busy
#define APP_RET_CONN_COUNT                  (ERROR_BASE_NUM + 16) ///< Maximum connection count exceeded.
#define APP_RET_RESOURCES                   (ERROR_BASE_NUM + 17) ///< Not enough resources for operation
#define APP_RET_TASK_ERROR                  (ERROR_BASE_NUM + 18) ///< Failure in starting or stopping task

#ifdef __cplusplus
}
#endif
#endif // ERROR_H__
