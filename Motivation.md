# Introduction #

Buffer overflow is a problem where a program tries to store a string of arbitrary length in a fixed size buffer, without checking for whether the string can fit inside the buffer. It results in inadvertently overwriting the memory location that follows the buffer. If the buffer resides in the data section, it could corrupt other global variables. If the buffer is heap allocated, it could corrupt data structure used by memory management routines. If the buffer resides on the stack, the overflow could overwrite the stack frame, including the return address, which can alter the program's control flow. In the context of networked programs, buffer overflow allows the execution of arbitrary code injected by a remote client or peer. This could result in compromise of sensitive data. If the buffer overflow happens in an operating system kernel, it could lead to privilege escalation. When remote code injection is combined with privilege escalation, it could result in the compromise of the whole system.

Any C program that manipulates strings may be prone to buffer overflow problem if care is not taken. In fact, we can argue that buffer overflow is almost a C language feature, inherent in the design of the C language. It has to do with the way strings are represented: a character array of indefinite length, until it is terminated by a `NUL` character (which has the numeric byte value of 0). C does not distinguish an array from a pointer, so string arguments to function typically bear the type `char *` or `const char *`. However, memory resources in C all have definite length. The programmer has to be careful when juggling possibly infinite sequence of characters in a finite amount of space.

Furthermore, the design of Standard C library routines for string manipulation adds insult to injury. We consider the following variants of string copy routines and point out why using them is error prone.

```
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
```

The `strcpy()` function copies the string from `src` to `dest` with the implicit assumption that destination is big enough to hold the `src` string. This will work if the destination buffer is at least `strlen(src) + 1` in size. However, many programmers fail to check for this. The next variant, `strncpy()`, copies at most `n` characters from `src` to `dest`, but the result in `dest` may not be `NUL` terminated. If the `src` string is longer than `n`, `dest` would not be a well-formed string. This variant is useless because, unless we already know `strlen(src)`, there is no way to tell whether `dest` is well-formed or not. We can always make sure `dest` is well-formed by placing a `NUL` character at the last byte of the `dest` buffer. However, many programmers neglect to do that as well. This could result in non-termination of string manipulation routines, even those that are correctly written.

String concatenation routines are similarly vulnerable. We consider the following variants of string concatenation routines and point out why they are error prone.

```
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
```

Both functions assume that dest already contains a well-formed string, though the string may be an empty string with only a `NUL` terminator. Both will append `src` string to the `dest` string. The `strcat()` function assumes that the destination will be large enough to hold both the existing string and the `src` string, i.e. the buffer must be at least `strlen(dest) + strlen(src) + 1`. The `strncat()` function will only copy at most `n` characters from `src`, but it still assumes that the destination is large enough to hold both the existing string plus `n` more characters, i.e. the buffer must be at least `strlen(dest) + n + 1`. Note that it is still a mistake to call `strncat(buf, src, sizeof(buf) - 1)`, unless `buf` stores the empty string. It is also a mistake to call `strcat()` or `strncat()` on a buffer that has not been initialized with a well-formed string, which may be an empty string. Note that a buffer containing the empty string only needs to have the first byte set to `NUL`, but the rest of the buffer can be uninitialized; it is not necessary to fill the rest of the buffer with `NUL`. However, at least it is guaranteed that `strcat()` and `strncat()` always `NUL` terminate the `dest` string.

For the reason that the existing string copying and concatenation library routines are easy to cause error, Todd C. Miller and Theo de Raadt [proposed a variant strlcpy() and strlcat()](http://www.usenix.org/event/usenix99/millert.html) and claimed them to be safer.

```
size_t strlcpy(char *dest, const char *src, size_t size);
size_t strlcat(char *dest, const char *src, size_t size);
```

Both functions take the buffer size as an argument, and return the length of the resulting string they tried to create. The strlcpy() function guarantees that `dest` is a `NUL` terminated, well-formed string. The strlcat() function only guarantees a well-formed string if `dest` already contains a well-formed string. In practice, it can be shown inductively that if you start with a well-formed string, and apply strlcat() repeatedly, then you will get a well-formed string regardless whether a truncation occurs due to buffer size limit.

Even so, it is still possible to misuse `strlcat()` and result in buffer overflow. It happens when a programmer is defeated by his own wit. He begins by realizing that repeatedly concatenating a list of strings to `strlcat()` takes O(n<sup>2</sup>) time. It is easier to see this if you imagine that all the strings have O(1) length, and there are n strings to concatenate.

```
const char *strings[] = {
  "eenie", "meenie", "miny", "moe"};

char buf[16];
buf[0] = '\0';
for (int i = 0; i < 4; i++)
  strlcat(buf, strings[i], sizeof(buf));
```

The reason is that `strlcat()` has to scan to the end of destination at each run in order to tell where to place the source string, which is an O(n) operation. The function is called n times, so the whole loop is O(n<sup>2</sup>). In order to make things run faster, he modifies the code as follows:

```
char *p = &buf;
*p = '\0';
for (int i = 0; i < 4; i++) {
  size_t advance = strlcat(p, strings[i], sizeof(buf));
  p += advance;
}
```

The idea is that after concatenation, he advances the pointer to the end of the destination string to do further concatenation. But this program results in buffer overflow error. The reason is that `strlcat()` starts counting `sizeof(buf)` from `p`, not from `buf`. His code gives `strlcat()` the wrong instruction that `p` will always have 16 more available bytes, despite the fact `buf` keeps filling up, and `p` keeps advancing towards the end of the buffer. We believe this problem is under-appreciated, and will likely surface under a careful code audit.

# The Solution #

Although the C language makes it easy to write programs with buffer overflow problems, there is still something the library can do to help the programmer make fewer mistakes. For this reason, we propose our variant `strxcpy()`, which combines both string copying and concatenation.

```
char *strxcpy(char *dest, const char *dest_end, const char *src, size_t n);
```

The function takes two parameters to define the buffer, `dest` and `dest_end`. It guarantees that it will not overwrite the memory location past `dest_end`, and the string at `dest` is always `NUL` terminated. Furthermore, it does not require `dest` to be a `NUL` terminated string. The return value is a character pointer to the `NUL` terminator of the `dest` string, so that we can easily append another string to it by calling `strxcpy()` again.

For example, the code before will be written as:

```
const char *strings[] = {
  "eenie", "meenie", "miny", "moe"};

char buf[16], *p = &buf;
for (int i = 0; i < 4; i++)
  p = strxcpy(p, buf + sizeof(buf), strings[i], SIZE_MAX);
```

Where `SIZE_MAX` is the maximum value of `size_t`; this can be obtained by `~(size_t) 0u`. The code is both fast and efficient.

Many library functions can benefit from the same idiom.

```
char *vsxprintf(char *dest, const char *dest_end, const char *fmt, va_list ap);
char *sxprintf(char *dest, const char *dest_end, const char *fmt, ...);
```

For example, `vsxprintf()` and `sxprintf()`, both variants of `vsnprintf()` and `snprint()`, allow easy concatenation of formatted string to the buffer, while the code is still resistant to buffer overflow problem. We can also define the `wcsxcpy()`, `vwcsxprintf()` and `wcsxprint()` similarly, for the `wchar_t` type.

We believe that using the `strxcpy()` library routine will dramatically improve the quality of C program that manipulates strings. Consider the following mistake that someone unfamiliar with strxcpy() might make.

```
/* Bad: using buffer directly and not updating dest. */
const char *foo = "this is foo ";
const char *bar = "and this is bar";

char buf[16];
strxcpy(buf, buf + sizeof(buf), foo, SIZE_MAX);
strxcpy(buf, buf + sizeof(buf), bar, SIZE_MAX);
```

In the above example, `buf` is filled with the `foo` string first, and then it gets overwritten by the `bar` string. The program produces an incorrect output, but there is no way to overrun the buffer.