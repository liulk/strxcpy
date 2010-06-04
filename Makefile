
CFLAGS += -Wall -Wextra -Werror

.PHONY: all clean

all::
clean::
	rm -f *~
	rm -f *.o

STRXCPY_SOURCES = \
	logging.c logging_format.c stringx.c

strxcpy.a: strxcpy.a($(STRXCPY_SOURCES:.c=.o))
	ranlib $@
all:: strxcpy.a
clean::
	rm -f strxcpy.a

logging_test: strxcpy.a
all:: logging_test
clean::
	rm -f logging_test
