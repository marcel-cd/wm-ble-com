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
 * @file    otap.h
 *
 */
#ifndef OTAP_H
#define OTAP_H

#include "app_app.h"

/** @brief  Magic number for the OTAP record
 *  !!! DO NOT change
 **/
#define OTAP_MAGIC                 0x2021


/** Applib persistent network
 *  !!! DO NOT remove any field previously defined
 *  New fields shall be added at the end of the structure
 */
typedef struct __attribute__ ((packed))
{
    uint16_t otap_record_magic;

} app_persistent_otap_t;


/** @brief  Initialize the OTAP module
 * must be called befor any other function
 *
 *  @return APP_RET_OK if successful
 */
int Otap_init(void);

/** @brief beforeWriting, the buffer has to be erased
 *
 */
int Otap_bufferBegin(void);

/** @brief when everything is ok, mark the buffer with the magic number
 *
 */
int Otap_bufferEnd(uint32_t totalLen, uint8_t sequence);

/**
 * @brief  Store the given data in the persistent memory
 * This will be done in blocks for 4 kByte because the flash has to be erased before writing
 *
 */
int Otap_bufferWrite(uint8_t * data, uint8_t len, uint32_t offset);

/** @brief After the Buffer has filled with data, call this function to
 * write into the Scratch Area
 *
 *
 * @warning: This function only works, if the stack has stopped
 *
 */
int Otap_process(void) ;

int Otap_bufferToScratch(void);

#endif
