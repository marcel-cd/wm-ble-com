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
 * @file    fsm.c
 * @brief   Main state machine
 *
 */

/* include global header first */
#include "app_app.h"
#include "app_settings.h"
#include "fsm.h"

#define DEBUG_LOG_MODULE_NAME "FSM"
#ifdef DEBUG_APP_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_APP_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"

/*****************************************************************************/
/*                                Declaration                                */
/*****************************************************************************/

/** max backlog of Statemachine Events */
#define EVENT_QUEUE_LEN sm_EVENT_QUEUE_LEN_FSM

/* ----------------------------------------------------------------------------*/
/* {{{ local members
 * ----------------------------------------------------------------------------*/

/**
 * use modules as singletons
 * malloc() is not available in WM-SDK, allocate the modules statically
 * modules will be allocated in app.c (or in the unit tests) */
static Sm_context donotuse_sm_context;
static sm_event_queue_t m_event_queue[EVENT_QUEUE_LEN];

/* }}} local memebers */

/* ----------------------------------------------------------------------------*/
/* {{{ tasks
 * ----------------------------------------------------------------------------*/
/** @name Tasks
 * @{ */

/** @brief Housekeeping Task
 * used for application periodic actions
 * */
static uint32_t heartbeat_task(void* context_p);
/** @brief Reboot the Node with some delay
 * */
static uint32_t reboot_task(void* context_p);

/** @} name Tasks */
/* }}} tasks */

/* ----------------------------------------------------------------------------*/
/* {{{ callbacks / handler
 * ----------------------------------------------------------------------------*/


/* }}} callbacks / hndler */

/* ----------------------------------------------------------------------------*/
/* {{{ state machine
 * ----------------------------------------------------------------------------*/
/** @name state machine effects and guards
 * @{ */

/** @brief state effect:
 *  - trigger app_E_INIT on all submodules
 *  - change fsm state to @ref fsm_S_SINGLE or fsm_S_MESH
 *  @warning call this effect only once in app lifetime
 *  @param context_p reference to the local context */
static void initialize(void* context_p);
static void reboot(void* context_p);
/**
 * @brief Definition of State Machine Event Matrix
 */
// *INDENT-OFF*
static const sm_event_matrix_t  m_event_matrix[] = {
    /* current state   event                guard,                   next state           entry action        exit action */
    {sm_S_BOOT,       sm_E_INIT,            NULL,                    sm_S_IDLE,           initialize,         NULL  },
    {sm_S_ANY_STATE,  fsm_E_REBOOT,         NULL,                    sm_S_NO_NEW_STATE,   reboot,             NULL  },
};
// *INDENT-ON*
/** @} */
/* }}} state machine */

/*****************************************************************************/
/*                              Local Functions                              */
/*****************************************************************************/

/* ----------------------------------------------------------------------------*/
/* {{{ helper
 * ----------------------------------------------------------------------------*/

/* }}} helper */

/* ----------------------------------------------------------------------------*/
/* {{{ state machine effects / guards
 * ----------------------------------------------------------------------------*/
/* }}} state machine effects / guards */

/* ----------------------------------------------------------------------------*/
/* {{{ callbacks / handler
 * ----------------------------------------------------------------------------*/

/* }}} callbacks / handler */
static void reboot(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    Fsm_context* fsm_context_p = (Fsm_context*) context_p;
    // reboot the node
    App_Scheduler_addTask_execTime_Caller(reboot_task, fsm_context_p, 5 * 1000, 500);
}

static void initialize(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    Fsm_context* fsm_context_p = (Fsm_context*) context_p;
    if (fsm_context_p->initialized) {
        LOG(LVL_ERROR, "initialize calld again");
        return;
    }

    // start Wirepas Stack
    AppSettings_configureNode(fsm_context_p->app_settings_p);
    if(lib_state->startStack()) {
        LOG(LVL_ERROR, "Failure in starting stack");
    }


    if (fsm_context_p->ble_context_p) {
        // by design Init the module
        Sm_fireEvent(
            fsm_context_p->ble_context_p->sm_context_p, sm_E_INIT, 500);
        // start scanning process
        Sm_fireEvent(
            fsm_context_p->ble_context_p->sm_context_p, ble_E_SCANNING_START, 500);
    } else {
        LOG(LVL_ERROR, "BLE module not initialized");
    }

    // start Ping Task
    App_Scheduler_addTask_execTime_Caller(heartbeat_task, fsm_context_p, 5 * 1000, 500);

    fsm_context_p->initialized = true;

    LOG(LVL_DEBUG, "initialize() done");
}

/* ----------------------------------------------------------------------------*/
/* {{{ tasks
 * ----------------------------------------------------------------------------*/
static uint32_t heartbeat_task(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    /* Fsm_context* fsm_context_p = (Fsm_context*) context_p; */

    /* if (fsm_context_p->app_settings_p->is_sink) { */
    /*     LOG(LVL_INFO, "Node is sink"); */
    /* } else { */
    /*     LOG(LVL_INFO, "Node is not sink"); */
    /* } */
    // do this ever 60 seconds
    return 5 * 1000;
}

static uint32_t reboot_task(void* context_p) {
    __ASSERT(NULL != context_p, "context_p must not be NULL");
    NVIC_SystemReset();
    return APP_SCHEDULER_STOP_TASK;
}

/* }}} tasks */

/*****************************************************************************/
/*                              Global Functions                             */
/*****************************************************************************/

/* ----------------------------------------------------------------------------*/
/* {{{ init
 * ----------------------------------------------------------------------------*/
extern void Fsm_createStatic(
    Fsm_context* const fsm_context_p,
    Ble_context* const ble_context_p,
    app_settings_t* app_settings_p
) {
    __ASSERT(NULL != fsm_context_p, "fsm_context_p must not be NULL");
    __ASSERT(NULL != app_settings_p, "app_settings_p must not be NULL");

    fsm_context_p->sm_context_p = &donotuse_sm_context;
    fsm_context_p->app_settings_p = app_settings_p;
    fsm_context_p->ble_context_p = ble_context_p;

    fsm_context_p->initialized = false;

    /* create the local fsm state machine */
    Sm_createStatic(fsm_context_p->sm_context_p,
                    fsm_context_p,
                    DEBUG_LOG_MODULE_NAME,
                    m_event_matrix,
                    NUM(m_event_matrix),
                    m_event_queue,
                    EVENT_QUEUE_LEN);

    if (ble_context_p) {
        ble_context_p->fsm_sm_context_p = fsm_context_p->sm_context_p;
        Ble_createStatic(ble_context_p, app_settings_p);
    }
}


extern void Fsm_destroyStatic(Fsm_context* const fsm_context_p) {
    if (!fsm_context_p) {
        return;
    }

    Sm_destroyStatic(fsm_context_p->sm_context_p);
}

/* }}} init */
