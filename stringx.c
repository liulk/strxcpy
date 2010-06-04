/* String buffer operations.
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

#include "stringx.h"
#include <stdarg.h>     /* va_list, va_start(), va_end() */
#include <stdio.h>      /* vsnprintf() */

char *strxcpy(char *dest, const char *dest_end, const char *src, size_t n)
{
  const char *stop = dest_end - 1;
  for ( ; dest < stop && n > 0 && *src != '\0'; n--)
    *dest++ = *src++;
  *dest = '\0';

  return dest;
}

char *vsxprintf(char *dest, const char *dest_end, const char *fmt, va_list ap)
{
  size_t max = dest_end - dest - 1;
  size_t len = vsnprintf(dest, max, fmt, ap);
  dest[max] = '\0';

  len = (len > max)? max : len;
  return dest + len;
}

char *sxprintf(char *dest, const char *dest_end, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *res = vsxprintf(dest, dest_end, fmt, ap);
  va_end(ap);
  return res;
}
