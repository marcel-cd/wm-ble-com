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

#include "sm.h"

#define DEBUG_LOG_MODULE_NAME "SM"
#ifdef DEBUG_APP_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_APP_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif
#include "debug_log.h"


void Sm_createStatic(Sm_context* me,
                     void * const module_context,
                     char * const module_name,
                     const sm_event_matrix_t* event_matrix,
                     uint8_t event_matrix_len,
                     sm_event_queue_t* event_queue,
                     uint8_t event_queue_len) {
    lib_system->enterCriticalSection();
    memset(event_queue, 0, sizeof(sm_event_queue_t)*event_queue_len);

    if (me != NULL) {
        Sm_init(me,
                module_context,
                module_name,
                event_queue,
                event_queue_len,
                event_matrix,
                event_matrix_len,
                Sm_handleEvents);
    } else {
        LOG(LVL_ERROR, "failed to malloc: %d", sizeof(Sm_context));
    }

    lib_system->exitCriticalSection();
}

/* operation Cleanup() */
void Sm_cleanup(Sm_context* const me) {
    UNUSED(me);
}

void Sm_destroyStatic(Sm_context* const me) {
    memset(me, 0, sizeof(Sm_context));
}

void Sm_destroy(Sm_context* const me) {
    if (me != NULL) {
        Sm_cleanup(me);
    }

    free(me->event_queue);
    free(me);
}

void Sm_init(Sm_context* const me,
             void * const module_context,
             char * const module_name,
             sm_event_queue_t* event_queue,
             uint8_t event_queue_len,
             const sm_event_matrix_t* event_matrix,
             uint8_t event_matrix_len,
             uint32_t (*handleEventsFunction)(void* const me)
            ) {

    sl_list_init(&me->event_queue_head);
    me->current_state = sm_S_BOOT;
    me->module_context = module_context;
    me->module_name = module_name;
    me->event_queue = event_queue;
    me->event_queue_len = event_queue_len;
    me->event_matrix = event_matrix;
    me->event_matrix_len = event_matrix_len;
    me->handleEvents = handleEventsFunction;
    me->exit_action = NULL;
}

void Sm_fireEvent(Sm_context* const me, sm_event_e event_type, uint32_t execution_time) {
    lib_system->enterCriticalSection();

    for (uint8_t i = 0; i < me->event_queue_len; i++) {
        if (me->event_queue[i].event == sm_E_NONE) {
            me->event_queue[i].event = event_type;
            sl_list_push_back(&me->event_queue_head, (sl_list_t*)&me->event_queue[i]);
            break;
        }
    }

    lib_system->exitCriticalSection();
    App_Scheduler_addTask_execTime_Caller(me->handleEvents, (void*)me, APP_SCHEDULER_SCHEDULE_ASAP, execution_time);
}


uint32_t Sm_handleEvents(void* me) {
    uint8_t i;
    sm_event_queue_t* event;
    bool transition_found = false;

    if (me == NULL) {
        return APP_SCHEDULER_STOP_TASK;
    }

    Sm_context* sm = (Sm_context*)me;

    if (sl_list_size(&sm->event_queue_head) == 0) {
        return APP_SCHEDULER_STOP_TASK;
    }

    lib_system->enterCriticalSection();
    event = (sm_event_queue_t*) sl_list_pop_front(&sm->event_queue_head);
    lib_system->exitCriticalSection();


    transition_found = false;

    // call entry function
    for (i = 0; i < sm->event_matrix_len; i++) {
        if ((event->event == sm->event_matrix[i].event) &&
                ((sm->current_state == sm->event_matrix[i].state) || (sm->event_matrix[i].state == sm_S_ANY_STATE)) &&
                ((sm->event_matrix[i].guard == NULL || (sm->event_matrix[i].guard)(sm->module_context)))) {

            LOG(LVL_INFO, "Trans(%s): %s -> %s (%s) (left %u)",
                sm->module_name,
                Sm_getStateName(sm->current_state),
                Sm_getStateName(sm->event_matrix[i].next_state),
                Sm_getEventName(event->event),
                sl_list_size(&sm->event_queue_head));

            // call exit action from last state
            if (sm->exit_action != NULL) {
                (sm->exit_action)(sm->module_context);
            }

            // call entry action from new state
            if (sm->event_matrix[i].entry_function != NULL) {
                (sm->event_matrix[i].entry_function)(sm->module_context);
            }

            // set current state
            if (sm->event_matrix[i].next_state != sm_S_NO_NEW_STATE) {
                sm->current_state = sm->event_matrix[i].next_state;
            }

            sm->exit_action = sm->event_matrix[i].exit_function;
            LOG(LVL_DEBUG, "State: %s",
                Sm_getStateName(sm->current_state));
            // break after the first match
            transition_found = true;
            break;
        }
    }


    // Free event slot
    event->event = sm_E_NONE;

    if (!transition_found) {
        LOG(LVL_DEBUG, "NO Trans(%s): %s (%s) (left %u)",
            sm->module_name,
            Sm_getStateName(sm->current_state),
            Sm_getEventName(event->event),
            sl_list_size(&sm->event_queue_head));
    }

    return (sl_list_size(&sm->event_queue_head) == 0) ? APP_SCHEDULER_STOP_TASK :
           APP_SCHEDULER_SCHEDULE_ASAP;
}
