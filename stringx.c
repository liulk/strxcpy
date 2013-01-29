/* String buffer operations.
 * Copyright (C) 2009--2013  Likai Liu <liulk@cs.bu.edu>
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

#define _GNU_SOURCE  /* strnlen() */

#include <assert.h>     /* assert() */
#include <stdarg.h>     /* va_list, va_start(), va_end() */
#include <stddef.h>     /* ptrdiff_t */
#include <stdint.h>     /* intmax_t, SIZE_MAX */
#include <stdio.h>      /* vsnprintf() */
#include <stdlib.h>     /* abort(), strtoul() */
#include <string.h>     /* strlen() */

#include "stringx.h"

char *strxcpy(char *dest, const char *dest_end, const char *src, size_t n) {
  const char *stop = dest_end - 1;
  for ( ; dest < stop && n > 0 && *src != '\0'; n--)
    *dest++ = *src++;
  *dest = '\0';

  return dest;
}

char *strxfromull(char *dest, const char *dest_end,
                  unsigned long long int x, int base, const char *digits) {
  assert(base >= 8 && base <= 16);
  assert(strnlen(digits, base) == (size_t) base);

  char buf[sizeof(unsigned long long) * 8 / 3 + 1];  /* max length for oct */
  char *p = buf;
  while (x) {
    *p++ = digits[x % base];
    x /= base;
  }

  if (p == buf)  /* in case x was zero to begin with */
    *p++ = '0';

  const char *stop = dest_end - 1;
  while (dest < stop && p > buf)
    *dest++ = *--p;
  *dest = '\0';

  return dest;
}

char *vsxprintf(char *dest, const char *dest_end,
                const char *fmt, va_list ap) {
  const char *stop = dest_end - 1;

  while (dest < stop) {
    char c = *fmt;

    if (c == '\0')
      break;

    if (c != '%') {
      *dest++ = c;
      ++fmt;
      continue;
    }

    /* begin format specification */

    c = *++fmt;

    if (c == '%') {
      *dest++ = '%';
      continue;
    }

    if (c >= '1' && c <= '9')
      abort();  /* argument selector is not implemented */

    char alt = 0, zero = 0, negw = 0, spc = 0, plus = 0, dec = 0;
    do {
      switch (c) {
      case '#': alt = 1; break;
      case '0': zero = 1; break;
      case '-': negw = 1; break;
      case ' ': spc = 1; break;
      case '+': plus = 1; break;
      case '\'': dec = 1; break;
      default:
        goto leave_flags;
      }
      c = *++fmt;
    } while (1);

  leave_flags:
    ;

    char sep = 0;
    switch (c) {  /* ignore separator characters for vectors */
    case ',': case ';': case ':': case '_':
      sep = c;
      c = *++fmt;
    }

    char *next_fmt;

    char width = strtoul(fmt, &next_fmt, 10);
    if (next_fmt == fmt)  /* no field width */
      width = -1;  /* unlimited width */
    fmt = next_fmt;
    c = *fmt;

    char prec = 0;
    if (c == '.') {  /* ignore precision */
      prec = strtoul(fmt + 1, &next_fmt, 10);
      fmt = next_fmt;
      c = *fmt;
    }

    /* parse length modifier */

    char len = sizeof(int);
    switch (c) {
    case 'h':
      c = *++fmt;
      if (c == 'h') {
        c = *++fmt;
        len = sizeof(char);
      } else
        len = sizeof(short int);
      break;
    case 'l':
      c = *++fmt;
      if (c == 'l') {
        c = *++fmt;
        len = sizeof(long long int);
      } else
        len = sizeof(long int);
      break;
    case 'j':
      c = *++fmt;
      len = sizeof(intmax_t);
      break;
    case 't':
      c = *++fmt;
      len = sizeof(ptrdiff_t);
      break;
    case 'z':
      c = *++fmt;
      len = sizeof(size_t);
      break;
    case 'q':
      abort();  /* quad_t is not supported */
      break;
    }

    ++fmt;

    /* since conversion is the last character, no need to c = *++fmt
     * beyond this point.
     */

    char convert = 1;

    /* integer conversion */

    char sign = 1;
    char base = 10;
    const char *digits = "0123456789abcdef";

    switch (c) {
    case 'd': case 'i': sign = 1; base = 10; break;
    case 'o': sign = 0; base = 8; break;
    case 'u': sign = 0; base = 10; break;

    case 'X': digits = "0123456789ABCDEF";  /* followed by 'x' */
    case 'x': sign = 0; base = 16; break;

    case 'D': len = sizeof(long int); sign = 1; base = 10; break;
    case 'O': len = sizeof(long int); sign = 0; base = 8; break;
    case 'U': len = sizeof(long int); sign = 0; base = 10; break;

    case 'p':
      dest = strxcpy(dest, dest_end, "0x", SIZE_MAX);
      len = sizeof(void *); sign = 0; base = 16; break;

    default:
      convert = 0;
    }

    if (convert) {
      long long int x;
      switch (len) {
      case sizeof(int8_t): x = va_arg(ap, int /* promoted */); break;
      case sizeof(int16_t): x = va_arg(ap, int /* promoted */); break;
      case sizeof(int32_t): x = va_arg(ap, int32_t); break;
      case sizeof(int64_t): x = va_arg(ap, int64_t); break;
      default: abort();
      }

      if (sign) {
        if (x < 0) {
          *dest++ = '-';
          if (dest >= stop) break;
        }
        x = -x;
      }

      dest = strxfromull(
          dest, dest_end, (unsigned long long int) x, base, digits);
      continue;
    }

    /* double conversion */

    char cap = c >= 'A' && c <= 'Z';
    char style = cap? c - 'A' + 'a' : c;  /* tolower */

    if (style == 'a' || style == 'e' || style == 'f' || style == 'g') {
      (void) va_arg(ap, double);
      dest = strxcpy(dest, dest_end, "<double>", SIZE_MAX);  /* placeholder */
      continue;
    }

    /* other conversion */

    switch (c) {
    case 'c':
      *dest++ = va_arg(ap, int /* promoted */);
      continue;
    case 's':
      dest = strxcpy(dest, dest_end, va_arg(ap, const char *), SIZE_MAX);
      continue;
    default:
      abort();
    }
  }

  *dest = '\0';
  return dest;
}

char *sxprintf(char *dest, const char *dest_end, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *res = vsxprintf(dest, dest_end, fmt, ap);
  va_end(ap);
  return res;
}

char *vsnxprintf(char *dest, const char *dest_end,
                 const char *fmt, va_list ap) {
  size_t max = dest_end - dest - 1;
  size_t len = vsnprintf(dest, max, fmt, ap);
  dest[max] = '\0';

  len = (len > max)? max : len;
  return dest + len;
}

char *snxprintf(char *dest, const char *dest_end, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *res = vsnxprintf(dest, dest_end, fmt, ap);
  va_end(ap);
  return res;
}
