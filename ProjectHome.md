The C language makes it easy to write programs with buffer overflow problems, but there is still something the library can do to help the programmer make fewer mistakes. For this reason, we propose strxcpy(), a variant of strcpy() and strcat(), which combines both string copying and concatenation.

```
char *strxcpy(char *dest, const char *dest_end, const char *src, size_t n);
```

The function takes two parameters to define the buffer, dest and dest\_end. It guarantees that it will not overwrite the memory location past dest\_end, and the string at dest is always NUL terminated. Furthermore, it does not require dest to be a NUL terminated string. The return value is a character pointer to the NUL terminator of the dest string, so that we can easily append another string to it by calling strxcpy() again.

We also propose the sprintf() variants using the same idiom.

```
char *vsxprintf(char *dest, const char *dest_end, const char *fmt, va_list ap);
char *sxprintf(char *dest, const char *dest_end, const char *fmt, ...);
```

See [the motivation](Motivation.md) for buffer overflow problems.