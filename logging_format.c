/* Logging facility formatter.
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

#include <string.h>
#include "stringx.h"

static char *
logging_append_info(char *dest, const char *dest_end,
                    logging_record_t *rec_p, const char *key, int key_len)
{
  /* Verify this list against the output of:
   * $ cat logging_variables.txt \
   *     | awk '{ print length($_), $_ }' \
   *     | sort -n
   */

  /* TODO(liulk): very low priority, rewrite the following section
   * using binary search.
   */

  size_t buf_size = dest_end - dest;

  switch (key_len) {
  case 4:
    if (strncmp(key, "name", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->name, buf_size);
    break;

  case 5:
    if (strncmp(key, "msecs", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%03d", rec_p->msecs);
    break;

  case 6:
    if (strncmp(key, "lineno", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%d", rec_p->lineno);
    else if (strncmp(key, "thread", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%u", rec_p->thread);
    break;

  case 7:
    if (strncmp(key, "levelno", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%d", rec_p->levelno);
    else if (strncmp(key, "created", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%f", rec_p->created);
    else if (strncmp(key, "asctime", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->asctime, buf_size);
    else if (strncmp(key, "process", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%d", rec_p->process);
    else if (strncmp(key, "message", key_len) == 0)
      dest = vsxprintf(dest, dest_end, rec_p->msg, rec_p->ap);
    break;

  case 8:
    if (strncmp(key, "pathname", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->pathname, buf_size);
    else if (strncmp(key, "filename", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->filename, buf_size);
    else if (strncmp(key, "funcName", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->func_name, buf_size);
    break;

  case 9:
    if (strncmp(key, "levelname", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->levelname, buf_size);
    break;

  case 10:
    if (strncmp(key, "threadName", key_len) == 0)
      dest = strxcpy(dest, dest_end, rec_p->thread_name, buf_size);
    break;

  case 15:
    if (strncmp(key, "relativeCreated", key_len) == 0)
      dest = sxprintf(dest, dest_end, "%f", rec_p->relative_created);
    break;
  }

  return dest;
}

static char *strchrnul(const char *s, int c) {
  for ( ; *s != '\0'; s++)
    if (*s == c)
      return (char *) s;
  return (char *) s;
}

size_t logging_formatter(logging_record_t *rec_p, const char *log_fmt,
                         char *buf, size_t buf_size)
{
  /* In the format parser below, we also use buf_size liberally as a
   * lose upper bound for number of bytes to copy.  This does not
   * cause the strx functions to overflow buffer.  The variable
   * dest_end is the guard that prevents overflow.
   */
  const char *dest_end = buf + buf_size;
  char *dest = buf;

  while (dest < dest_end - 1) {
    /* Copy the format string verbatim until the next occurrence of '%'. */
    const char *ahead = strchrnul(log_fmt, '%');
    dest = strxcpy(dest, dest_end, log_fmt, ahead - log_fmt);
    if (*ahead == '\0')
      break;

    /* Handle format specification, somewhat Python style.  We do not
     * support any modifiers.
     */
    log_fmt = ahead + 1;

    if (*log_fmt == '\0')
      break;
    else if (*log_fmt == '(') {
      ahead = strchrnul(log_fmt, ')');
      if (ahead == '\0')
	break;

      const char *subject = log_fmt + 1;
      int len = ahead - subject;

      char type = *(ahead + 1);
      if (type == 's' || type == 'd' || type == 'f') {
        dest = logging_append_info(dest, dest_end, rec_p, subject, len);
        ahead += 2;
      } else {
        /* Not a format specification.  Ignore. */
        dest = strxcpy(dest, dest_end, log_fmt - 1, ahead - log_fmt + 2);
        ahead += 1;
      }

      log_fmt = ahead;
    }
    else {
      dest = strxcpy(dest, dest_end, log_fmt, 1);
      log_fmt++;
    }
  }

  dest = strxcpy(dest, dest_end, "\n", 1);

  return dest - buf;
}
