/* Logging facility.
 * Copyright (C) 2009, 2010  Likai Liu <liulk@cs.bu.edu>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Provides application a simple logging facility similar to Python's
 * logging module.  Runs much faster and integrates well with C, but
 * this doesn't have nearly as much log routing support.
 *
 * The end-user can alter logging output by modifying these
 * environment variables:
 *
 *   - LOGGING_LOG_LEVEL: a numeric value (TODO: also recognize symbolic name)
 *
 *   - LOGGING_LOG_FORMAT: a Python style format string.  See:
 *     http://docs.python.org/library/logging.html#formatter-objects
 *
 *   - LOGGING_TIME_FORMAT: a strftime() format string for displaying
 *     human readable time.
 *
 * We also recognize the following per-compilation-unit compile-time
 * options:
 *
 *   - _DISABLE_LOGGING: will not output any logs; does not affect
 *     conditional logging.
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>      /* FILE */
#include <stdarg.h>     /* va_list */

/**********************************************************************/
/* Section: Typical Uses                                              */
/**********************************************************************/

/* Log levels. */

#define LOG_NOTSET      0
#define LOG_DEBUG       10
#define LOG_INFO        20
#define LOG_WARN        30
#define LOG_ERROR       40
#define LOG_CRIT        50

#define LOG_WARNING     LOG_WARN
#define LOG_CRITICAL    LOG_CRIT

/* Logging can be disabled in the current compilation unit by defining
 * _DISABLE_LOGGING or ISABLE_LOGGING (i.e. -DISABLE_LOGGING).
 */

#if defined(_DISABLE_LOGGING) || defined(ISABLE_LOGGING)
#  define LOG(level, fmt, args...)
#else
#  define LOG(level, fmt, args...) \
     logging_raise(__FILE__, __LINE__, __func__, level, fmt, ##args)
#endif  /* _DISABLE_LOGGING || ISABLE_LOGGING */

#define DEBUG(fmt, args...)     LOG(LOG_DEBUG, fmt, ##args)
#define INFO(fmt, args...)      LOG(LOG_INFO, fmt, ##args)
#define WARN(fmt, args...)      LOG(LOG_WARN, fmt, ##args)
#define ERROR(fmt, args...)     LOG(LOG_ERROR, fmt, ##args)
#define CRIT(fmt, args...)      LOG(LOG_CRIT, fmt, ##args)

/* Convenience macro for logging the run-time evaluation of an
 * expression.
 */

#define DEBUG_EXPR(fmt, e, args...) \
  LOG(LOG_DEBUG, "%s = " fmt, #e, e, ##args)

/* Conditional logging are not disabled by _DISABLE_LOGGING.  They
 * provide finer granularity of control whether log should be raised
 * or not.  Recommended use:
 *
 * #define KEYB_EVENT 1        // Enable logging of keyboard events.
 * #define MOUSE_EVENT 0       // Disable logging of mouse events.
 *
 * void do_keypress()
 * {
 *   LOG_IF(KEYB_EVENT, level, ...);
 * }
 *
 * void do_mouse()
 * {
 *   LOG_IF(MOUSE_EVENT, level, ...);
 * }
 */

#define LOG_IF(cond, level, fmt, args...)                               \
  do {                                                                  \
    if (cond)                                                           \
      logging_raise(__FILE__, __LINE__, __func__, level, fmt, ##args);  \
  } while(0)

#define DEBUG_IF(cond, fmt, args...)    LOG_IF(cond, LOG_DEBUG, fmt, ##args)
#define INFO_IF(cond, fmt, args...)     LOG_IF(cond, LOG_INFO, fmt, ##args)
#define WARN_IF(cond, fmt, args...)     LOG_IF(cond, LOG_WARN, fmt, ##args)
#define ERROR_IF(cond, fmt, args...)    LOG_IF(cond, LOG_ERROR, fmt, ##args)
#define CRIT_IF(cond, fmt, args...)     LOG_IF(cond, LOG_CRIT, fmt, ##args)

/**********************************************************************/
/* Section: Advanced Uses                                             */
/**********************************************************************/

/* The logging record is modeled after Python logging module.  See:
 * http://docs.python.org/library/logging.html#formatter-objects
 * except that capitalized letters have been translated to lowercase
 * with a prepended underscore.
 */

typedef struct logging_record_s {
  const char *name;
  int levelno;
  const char *levelname;
  const char *pathname;
  const char *filename;
  const char *func_name;
  int lineno;
  double created;
  double relative_created;
  const char *asctime;
  int msecs;
  unsigned long thread;
  const char *thread_name;
  int process;
  const char *msg;
  va_list ap;
} logging_record_t;

/* By default, the log emitter formats the log entry according to the
 * format set by the environment variable LOGGING_LOG_FORMAT and
 * writes the result to stdlog.  The emitter can be overridden to
 * another function that acts upon a logging record.
 */

typedef void (*logging_emit_func_t)(logging_record_t *rec_p);

extern logging_emit_func_t logging_emitter;

/* The stdlog file can also be redirected if so desired. */

extern FILE *stdlog;

/* Global log level can be overridden. */

extern int logging_log_level;

/* Global log entry format and time format can be replaced if so desired. */

extern const char *logging_log_format;
extern const char *logging_time_format;

/* Functions that most users don't really need to know. */

void logging_ensure_initialized();
void logging_raise(const char *file, int line, const char *func, int log_level,
                   const char *fmt, ...);
void logging_emit_stdlog(logging_record_t *rec_p);
size_t logging_formatter(logging_record_t *rec_p, const char *log_fmt,
                         char *buf, size_t buf_size);

/**********************************************************************/
/* Section: Internal Use Only                                         */
/**********************************************************************/

/* __FILE__ and __LINE__ are standard predefined macros of C
 * preprocessor.  See:
 * http://www.delorie.com/gnu/docs/gcc/cpp_21.html
 */

/* __func__ is a built-in variable recognized by C99 standard.  See:
 * http://gcc.gnu.org/onlinedocs/gcc-3.4.0/gcc/Function-Names.html
 */

#if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2
#    define __func__ __FUNCTION__
#  else
#    define __func__ "<unknown function>"
#  endif
#endif

#endif  /* __LOGGING_H__ */
