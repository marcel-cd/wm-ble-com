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

#ifndef ___ASSERT_H_
#define ___ASSERT_H_

#include <stdbool.h>

#ifdef CONF_ASSERT
#ifndef __ASSERT_ON
#define __ASSERT_ON CONF_ASSERT_LEVEL
#endif
#endif

#ifdef CONF_FORCE_NO_ASSERT
#undef __ASSERT_ON
#define __ASSERT_ON 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Wrapper around printk to avoid including printk.h in assert.h */
void assert_print(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

#if defined(CONF_ASSERT_VERBOSE)
#define __ASSERT_PRINT(fmt, ...) assert_print(fmt, ##__VA_ARGS__)
#else /* CONF_ASSERT_VERBOSE */
#define __ASSERT_PRINT(fmt, ...)
#endif /* CONF_ASSERT_VERBOSE */

#ifdef CONF_ASSERT_NO_MSG_INFO
#define __ASSERT_MSG_INFO(fmt, ...)
#else  /* CONF_ASSERT_NO_MSG_INFO */
#define __ASSERT_MSG_INFO(fmt, ...) __ASSERT_PRINT("\t" fmt "\n", ##__VA_ARGS__)
#endif /* CONF_ASSERT_NO_MSG_INFO */

#if !defined(CONF_ASSERT_NO_COND_INFO) && !defined(CONF_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                              \
	__ASSERT_PRINT("ASSERTION FAIL [%s] @ %s:%d\n", \
		Z_STRINGIFY(test),                      \
		__FILE__, __LINE__)
#endif

#if defined(CONF_ASSERT_NO_COND_INFO) && !defined(CONF_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                         \
	__ASSERT_PRINT("ASSERTION FAIL @ %s:%d\n", \
		__FILE__, __LINE__)
#endif

#if !defined(CONF_ASSERT_NO_COND_INFO) && defined(CONF_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                      \
	__ASSERT_PRINT("ASSERTION FAIL [%s]\n", \
		Z_STRINGIFY(test))
#endif

#if defined(CONF_ASSERT_NO_COND_INFO) && defined(CONF_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                 \
	__ASSERT_PRINT("ASSERTION FAIL\n")
#endif

#ifdef __ASSERT_ON
#if (__ASSERT_ON < 0) || (__ASSERT_ON > 2)
#error "Invalid __ASSERT() level: must be between 0 and 2"
#endif

#if __ASSERT_ON

#ifdef __cplusplus
extern "C" {
#endif


void assert_post_action(void);
#define __ASSERT_POST_ACTION() assert_post_action()

#ifdef __cplusplus
}
#endif

#define __ASSERT_NO_MSG(test)                                             \
	do {                                                              \
		if (!(test)) {                                            \
			__ASSERT_LOC(test);                               \
			__ASSERT_POST_ACTION();                           \
		}                                                         \
	} while (false)

#define __ASSERT(test, fmt, ...)                                          \
	do {                                                              \
		if (!(test)) {                                            \
			__ASSERT_LOC(test);                               \
			__ASSERT_MSG_INFO(fmt, ##__VA_ARGS__);            \
			__ASSERT_POST_ACTION();                           \
		}                                                         \
	} while (false)

#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...)                \
	do {                                                       \
		expr2;                                             \
		__ASSERT(test, fmt, ##__VA_ARGS__);                \
	} while (false)

#if (__ASSERT_ON == 1)
#warning "__ASSERT() statements are ENABLED"
#endif
#else
#define __ASSERT(test, fmt, ...) { }
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#define __ASSERT_NO_MSG(test) { }
#endif
#else
#define __ASSERT(test, fmt, ...) { }
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#define __ASSERT_NO_MSG(test) { }
#endif

#endif /* ___ASSERT_H_ */
