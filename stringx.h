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

/* A string buffer is a region of memory between two pointers, the
 * destination ("dest") and the end of the buffer ("dest_end").  The
 * buffer size is exactly (dest_end - dest) bytes.  The content of the
 * string buffer is always guaranteed to be zero terminated.
 *
 * The functions declared in this header file are variations to the
 * Standard C string functions that operate on string buffers.
 *
 * String buffers are easy to manipulate safely.  All functions return
 * the pointer to the zero terminator, so that new content can be
 * easily accumulated to the end of the string.
 */

#ifndef __STRINGX_H__
#define __STRINGX_H__

#include <stdarg.h>     /* va_list */
#include <stddef.h>     /* size_t */

/* Copies/appends a string from src to the string buffer denoted by
 * dest and dest_end.  The function copies at most n characters, until
 * the end of src, or until the string buffer is full, whichever
 * occurs the earliest.
 */
char *strxcpy(char *dest, const char *dest_end, const char *src, size_t n);

/* Formats a string into the string buffer, truncated to the capacity
 * of the buffer.  The vsxprintf() variant takes an argument pointer,
 * and the sxprintf variant is a variadic function that takes any
 * number of arguments on stack.
 */
char *vsxprintf(char *dest, const char *dest_end, const char *fmt, va_list ap);
char *sxprintf(char *dest, const char *dest_end, const char *fmt, ...);

/* TODO(liulk): make wcs* variants available too. */

#endif  /* __STRINGX_H__ */
