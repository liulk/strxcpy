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

#include "logging.h"

#include <math.h>       /* modf() */
#include <pthread.h>    /* pthread_self() */
#include <stdarg.h>     /* va_list, va_start(), va_copy(), va_end() */
#include <stdio.h>      /* fdopen(), fopen(), fwrite(), perror() */
#include <stdlib.h>     /* abort(), getenv(), strtol() */
#include <string.h>     /* strrchr() */
#include <sys/time.h>   /* gettimeofday() */
#include <time.h>       /* strftime(), localtime() */
#include <unistd.h>     /* STDERR_FILENO, getpid() */

#define LOGFILE_OPEN_MODE       "a"
FILE *stdlog = NULL;
logging_emit_func_t logging_emitter = NULL;

static int g_log_level = LOG_NOTSET;

static const char *g_log_format =
  "%(asctime)s - %(levelname)s - %(message)s";

static const char *g_time_format =
  "%Y-%m-%d %H:%M:%S";

static double g_init_time = 0.0;

static double gettimeofday_double()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
}

static void logging_init_using_stderr()
{
  int new_stderr_fd = dup(STDERR_FILENO);
  if (new_stderr_fd != -1)
    stdlog = fdopen(new_stderr_fd, LOGFILE_OPEN_MODE);
  return;
}

static void logging_init_using_file(const char *file)
{
  stdlog = fopen(file, LOGFILE_OPEN_MODE);
}

void logging_emit_stdlog(logging_record_t *rec_p)
{
  char buf[1024];
  size_t c = logging_formatter(rec_p, g_log_format, buf, sizeof(buf));
  fwrite(buf, c, 1, stdlog);
}

void logging_ensure_initialized()
{
  if (stdlog != NULL)
    return;

  if (!logging_emitter)
    logging_emitter = logging_emit_stdlog;

  /* TODO(liulk): retry only after x seconds. */

  const char *res;

  if ((res = getenv("LOGGING_LOG_FORMAT")) != NULL)
    g_log_format = res;

  if ((res = getenv("LOGGING_TIME_FORMAT")) != NULL)
    g_time_format = res;

  if ((res = getenv("LOGGING_LOG_LEVEL")) != NULL)
    g_log_level = strtol(res, (char **) NULL, 10);

  if ((res = getenv("LOGGING_LOG_FILE")) != NULL)
    logging_init_using_file(res);

  /* If logging_init_using_file() did not initialize, */
  if (stdlog == NULL)
    logging_init_using_stderr();

  /* If logging_init_using_stderr() also did not initialize successfully, */
  if (stdlog == NULL) {
    perror("!!! LOGGING IS DISABLED");
    return;
  }

  setbuf(stdlog, NULL);  /* Set to unbuffered mode like stderr. */

  g_init_time = gettimeofday_double();
}

typedef struct {
  int key;
  const char *data;
} int_sz_pair_t;

#define LOG_LEVEL_ENTRY(name) { LOG_ ## name, #name }

static const int_sz_pair_t k_log_level_names[] = {
  LOG_LEVEL_ENTRY(NOTSET),
  LOG_LEVEL_ENTRY(DEBUG),
  LOG_LEVEL_ENTRY(INFO),
  LOG_LEVEL_ENTRY(WARN),
  LOG_LEVEL_ENTRY(ERROR),
  LOG_LEVEL_ENTRY(CRIT),
};

#define countof(var) (sizeof(var) / sizeof(var[0]))

static const int k_num_log_level_names = countof(k_log_level_names);

static const char *string_of_log_level(int log_level)
{
  int i;
  for (i = 0; i < k_num_log_level_names; i++)
    if (k_log_level_names[i].key >= log_level)
      return k_log_level_names[i].data;

  return "UNDEFINED";
}

void logging_vprintf(const char *pathname, int lineno, const char *func_name,
                     int levelno, const char *msg, va_list ap)
{
  logging_record_t r;

  r.name = "root";

  /* Prepare level number and name string. */
  r.levelno = levelno;
  r.levelname = string_of_log_level(levelno);

  /* Prepare pathname, filename, func_name strings and the line number. */
  r.pathname = pathname;

  const char *tmpsz;
  tmpsz = strrchr(pathname, '/');
  r.filename = (tmpsz == NULL)? pathname : tmpsz + 1;

  r.func_name = func_name;
  r.lineno = lineno;

  /* Prepare created (seconds) and relative created (milliseconds) time. */
  r.created = gettimeofday_double();
  r.relative_created = (r.created - g_init_time) * 1000.0;

  /* Prepare asctime string. */
  char asctime_buf[128];

  time_t t = time(NULL);
  struct tm tms;
  localtime_r(&t, &tms);

  size_t res = strftime(asctime_buf, sizeof(asctime_buf), g_time_format, &tms);
  if (res >= sizeof(asctime_buf))
    res = sizeof(asctime_buf) - 1;
  asctime_buf[res] = '\0';

  r.asctime = asctime_buf;

  /* Prepare milliseconds in integer. */
  double tmpd;
  r.msecs = (int) (modf(r.created, &tmpd) * 1000.0);

  /* Prepare thread id, thread name, and process id. */
  r.thread = (unsigned long) pthread_self();
  r.thread_name = "UnknownThread";
  r.process = getpid();

  /* Prepare logged message. */
  r.msg = msg;
  va_copy(r.ap, ap);

  logging_emitter(&r);

  va_end(r.ap);
}

void logging_printf(const char *pathname, int lineno, const char *func_name,
                    int levelno, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  logging_vprintf(pathname, lineno, func_name, levelno, fmt, ap);
  va_end(ap);
}

void logging_raise(const char *file, int line, const char *func, int log_level,
                   const char *fmt, ...)
{
  logging_ensure_initialized();
  if (log_level < g_log_level)
    return;

  va_list ap;
  va_start(ap, fmt);
  logging_vprintf(file, line, func, log_level, fmt, ap);
  va_end(ap);
}
