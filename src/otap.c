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

#include "otap.h"
#define DEBUG_LOG_MODULE_NAME "OTAP"
#ifdef DEBUG_APP_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_APP_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"

// from pca10100_scratchpad.ini file
#define OTAP_PERSISTENT_MEMORY_AREA_ID 0x8AE573BB

// Randomly generated to consider area correctly initialized
#define OTAP_PERSISTENT_MAGIC 0x1E7B3ABA
#define BLOCK_SIZE 512
/** buffer used in lib_memory_area->startWrite()  */
uint8_t m_buffer_block_write[BLOCK_SIZE];
/** used to hold data, till its been written, memcpy to otap_buffer_block_write
 */
/* uint8_t m_buffer_block_cache[BLOCK_SIZE]; */
/** this counter will be reseted in Otap_begin() and marks the actual position
 * in writing */
/* static uint16_t m_write_block_number = 0; */

static size_t m_header_size;

static bool m_initialized = false;

static size_t m_usable_memory_size;

static app_lib_mem_area_info_t m_memory_area;

static bool active_wait_for_end_of_operation(int32_t timeout_us) {
  app_lib_time_timestamp_hp_t timeout_end;
  bool busy, timeout_reached;

  timeout_end =
      lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(), timeout_us);

  /* Wait for flash to be ready */
  do {
    busy = lib_memory_area->isBusy(OTAP_PERSISTENT_MEMORY_AREA_ID);
    timeout_reached =
        lib_time->isHpTimestampBefore(timeout_end, lib_time->getTimestampHp());
  } while (busy == true && !timeout_reached);

  return !busy;
}

static bool write(uint32_t to, void* from, size_t amount) {
  int32_t timeout_us;

  if (lib_memory_area->startWrite(OTAP_PERSISTENT_MEMORY_AREA_ID, to, from,
                                 amount) != APP_LIB_MEM_AREA_RES_OK) {
    return false;
  }

  timeout_us = ((m_memory_area.flash.byte_write_time +
              m_memory_area.flash.byte_write_call_time) *
             amount) *
            2;

  /* Wait end of read */
  return active_wait_for_end_of_operation(timeout_us);
}

static bool read(void *to, uint32_t from, size_t amount) {
  int32_t timeout_us;

  if (lib_memory_area->startRead(OTAP_PERSISTENT_MEMORY_AREA_ID, to, from,
                                 amount) != APP_LIB_MEM_AREA_RES_OK) {
    return false;
  }

  /* Compute timeout depending of external/internal flash */
  if (m_memory_area.external_flash) {
    // Most of time for external flash is on bus (SPI or I2C)
    // And probably already spent in StartRead.
    // Take a large timeout that should never be reached
    timeout_us = 100000; // 100ms
  } else {
    // Read access in internal flash are synchronous
    timeout_us = 0;
  }

  /* Wait end of read */
  return active_wait_for_end_of_operation(timeout_us);
}

int Otap_init(void) {
  if (m_initialized) {
    return APP_RET_OK;
  }

  if (lib_memory_area->getAreaInfo(OTAP_PERSISTENT_MEMORY_AREA_ID,
                                   &m_memory_area) != APP_LIB_MEM_AREA_RES_OK) {
    return APP_PERSISTENT_RES_NO_AREA;
  }

  // Header size must be at least 3*uint32_t (magic_size + data_len + sequence)
  // size and a multiple of writable flash size to keep next region alligned
  // too. We assume that write_aligment is either lower than sizeof(uint32_t) or
  // a multiple of it.
  m_header_size = m_memory_area.flash.write_alignment > 3 * sizeof(uint32_t)
                      ? m_memory_area.flash.write_alignment
                      : 3 * sizeof(uint32_t);

  m_usable_memory_size = m_memory_area.area_size - m_header_size;

  m_initialized = true;
  return APP_RET_OK;
}

int Otap_bufferBegin() {
  size_t erase_page_size = m_memory_area.flash.erase_sector_size;
  uint32_t sector_base = 0;
  uint32_t timeout;
  // Erase the minimum number of blocks for a given area
  size_t num_page = (m_usable_memory_size / erase_page_size);
  // Copy it as next function will update it
  size_t num_page_temp = num_page;
  // reset the block counter for the write method
  /* m_write_block_number = 0; */

  if (!m_initialized) {
    return APP_PERSISTENT_RES_UNINITIALIZED;
  }

  if (lib_memory_area->startErase(OTAP_PERSISTENT_MEMORY_AREA_ID, &sector_base,
                                  &num_page_temp) != APP_LIB_MEM_AREA_RES_OK) {
    return APP_PERSISTENT_RES_FLASH_ERROR;
  }

  // Determine the timeout dynamically with 100% margin (x2)
  timeout = (m_memory_area.flash.sector_erase_time * num_page) * 2;

  // Wait end of erase
  if (!active_wait_for_end_of_operation(timeout)) {
    return APP_PERSISTENT_RES_ACCESS_TIMEOUT;
  }

  return APP_RET_OK;
}

int Otap_bufferEnd(uint32_t totalLen, uint8_t sequence) {
  uint32_t magic = OTAP_MAGIC;

  // Write Header data:
  memcpy(m_buffer_block_write, &magic, sizeof(magic));
  memcpy(m_buffer_block_write + sizeof(magic), &totalLen, sizeof(totalLen));
  memcpy(m_buffer_block_write + sizeof(magic) + sizeof(totalLen), &sequence,
         sizeof(sequence));

  if (!write(0, m_buffer_block_write, m_header_size)) {
    return APP_PERSISTENT_RES_FLASH_ERROR;
  }

  return APP_RET_OK;
}
int Otap_bufferWrite(uint8_t *data, uint8_t len, uint32_t offset) {

  if (!m_initialized) {
    return APP_PERSISTENT_RES_UNINITIALIZED;
  }

  if (len > m_usable_memory_size) {
    return APP_PERSISTENT_RES_TOO_BIG;
  }

  if(!write(offset + m_header_size, data, len)) {
    return APP_PERSISTENT_RES_FLASH_ERROR;
  }
  /* memcpy(&m_buffer_block_write[m_header_size + offset], data, len); */
  return APP_RET_OK;


}

uint8_t m_test[16];
int Otap_process() {
  uint32_t magic = OTAP_MAGIC;
  uint32_t len = 0;
  size_t block_size = 0;
  uint8_t sequence = 0;

  int ret = APP_RES_OK;

  if (!m_initialized) {
    LOG(LVL_ERROR, "OTAP not initialized");
    return APP_PERSISTENT_RES_UNINITIALIZED;
  }

  read(m_buffer_block_write, 0, m_header_size);

  if (memcmp(m_buffer_block_write, &magic, sizeof(magic)) != 0) {
    LOG(LVL_ERROR, "Magic number not found");
    return -1;
  }

  // len has to be in miniumum 96 bytes an devidable by 4
  len = *((uint32_t *)(m_buffer_block_write + sizeof(magic)));
  if (len % 4 != 0 || len < 96) {
    LOG(LVL_ERROR, "Invalid length");
    return -1;
  }

  // len has to be in miniumum 96 bytes an devidable by 4
  sequence = *((uint8_t *)(m_buffer_block_write + sizeof(magic) + sizeof(len)));
  LOG(LVL_INFO, "Otap_process: len=%d, sequence=%d", len, sequence);

  // update works only, if wirepas stack is not running
  if (lib_state->getStackState() == APP_LIB_STATE_STARTED) {
    LOG(LVL_ERROR, "Stack is running");
    return -1;
  }

  if (lib_otap->begin(len, sequence) != APP_RES_OK) {
    LOG(LVL_ERROR, "otap begin failed");
    return APP_PERSISTENT_RES_FLASH_ERROR;
  }

  read(m_test, m_header_size + len-16, 16);
  LOG_BUFFER(LVL_INFO, m_test, 16);

  block_size = BLOCK_SIZE;
  if (block_size > lib_otap->getMaxBlockNumBytes()) {
    block_size = lib_otap->getMaxBlockNumBytes();
  }
  // write data to the scratchpad area:
  for (size_t i = 0; i < len; i += block_size) {
    read(m_buffer_block_write, m_header_size + i, block_size);
    if ((i + block_size) >= len) {
      LOG(LVL_INFO, "Otap_process: last block,  %d/ %d",i,  len - i);
      /* LOG_BUFFER(LVL_INFO, m_buffer_block_write, 16); */
    }
    ret = lib_otap->write(i, ((len - i) > block_size) ? block_size : len - i,
                          m_buffer_block_write);
    if (ret != APP_LIB_OTAP_WRITE_RES_OK  && ret != APP_LIB_OTAP_WRITE_RES_COMPLETED_OK) {
      LOG(LVL_ERROR, "otap (%d) write failed %d", i, ret);
      return -1;
    }
  }

  ret = lib_otap->setTargetScratchpadAndAction(lib_otap->getSeq(),
                                           lib_otap->getCrc(),
                                           APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS,
                                           0 // Not used for this action);
                                           );
  // trigger all non-sink nodes to update:
  if (ret != APP_RES_OK) {
    LOG(LVL_ERROR, "otap setTargetScratchpadAndAction failed %d", ret);
    return ret;
  }

  // read status and wait till all nodes are updated

  // now we can update the Sink node itself:
  ret = lib_otap->setToBeProcessed();
  if (ret != APP_RES_OK) {
    LOG(LVL_ERROR, "otap setToBeProcessed failed %d", ret);;
    return -1;
  }

  return APP_RET_OK;
}

int Otap_bufferToScratch(void) { return APP_RES_OK; }
