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
 * @file    sm.h
 * @brief   Header for the State Machine
 *
 */
#ifndef SM_H_
#define SM_H_

#include "app_app.h"

/* "class" StateMachine */
typedef struct Sm Sm_context;

/** The structure that describes events, and is sent on a queue to the TCP/IP task. */
typedef struct {
    sl_list_t list;
    sm_event_e event;
    uint32_t execution_time;
} sm_event_queue_t;

/** Function Pointer for state_table */
typedef void (*functionPointerPacketType)(void* context_p);
/** Function Pointer for guard */
typedef bool (*guard_cb_fp)(void* context_p);

/** Event Matrix Event Table:
 * Design the State Machine Diagram UML in code
 * Filter events and define effects/actions */
typedef struct {
    /** state filter,
     * one can use @ref app_S_ANY_STATE, if you want to match to all states */
    sm_state_e state;
    /** event filter */
    sm_event_e event;
    /** guard callback function */
    guard_cb_fp guard;
    /** when entry matches, set current state to this one,
     * use @ref app_S_NO_NEW_STATE, if state shall not be changed */
    sm_state_e next_state;
    /** entry action/effet of the new state,
     * use NULL, if not needed */
    functionPointerPacketType entry_function;
    /** exit action/effect of the state,
     * use NULL, if not needed */
    functionPointerPacketType exit_function;
} sm_event_matrix_t;

/** @brief Instance to the local StateMachin "Class"
 */
struct Sm {
    void * module_context;
    char * module_name;
    volatile sm_state_e current_state;
    sm_event_queue_t *event_queue;
    uint8_t event_queue_len;
    sl_list_head_t event_queue_head;
    const sm_event_matrix_t *event_matrix;
    uint8_t event_matrix_len;
    functionPointerPacketType exit_action;
    uint32_t (*handleEvents)(void* const me);
};

/** @brief Create State machine instance with malloc
 *
 * Use this function to create a instance, if the malloc is supported
 *
 * @param   event_matrix
 *          the rules for the state machine
 * @param   event_matrix_len
 *          number of rules/elements
 * @param   event_queue_len
 *          maximum number of event backlog
 * @return  Sm
 *          Pointer to the instance or null, if malloc failed
 */
Sm_context * Sm_create(const sm_event_matrix_t *event_matrix,
               uint8_t event_matrix_len,
               uint8_t event_queue_len);

/** @brief Destroy State machine instance created with Sm_create(...)
 *
 * Use this function to destroy an instance with malloc
 * @param me
 *        Pointer to the instance
 */
void Sm_destroy(Sm_context* const me);

/* Init state machine (no malloc)  */
void Sm_createStatic(Sm_context* const me,
                     void * const module_context,
                     char * const module_name,
                     const sm_event_matrix_t *event_matrix,
                     uint8_t event_matrix_len,
                     sm_event_queue_t *event_queue,
                     uint8_t event_queue_len);
void Sm_destroyStatic(Sm_context* const me);
void Sm_cleanup(Sm_context* const me);
void Sm_init(Sm_context* const me,
             void * const module_context,
             char * const module_name,
             sm_event_queue_t *event_queue,
             uint8_t event_queue_len,
             const sm_event_matrix_t *event_matrix,
             uint8_t event_matrix_len,
             uint32_t (*handle_events_function)(void* const me)
             );

uint32_t Sm_handleEvents(void * me);
void Sm_fireEvent(Sm_context* const me, sm_event_e event_type, uint32_t execution_time);


#endif // SM_H_
