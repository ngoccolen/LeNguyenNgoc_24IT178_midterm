CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -D_NETBSD_SOURCE -D_POSIX_C_SOURCE=200809L

TARGET = ls
OBJS = main.o options.o filesystem.o sort.o display.o

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS}

main.o: main.c options.h filesystem.h sort.h display.h
	${CC} ${CFLAGS} -c main.c -o main.o

options.o: options.c options.h
	${CC} ${CFLAGS} -c options.c -o options.o

filesystem.o: filesystem.c filesystem.h options.h
	${CC} ${CFLAGS} -c filesystem.c -o filesystem.o

sort.o: sort.c sort.h filesystem.h options.h
	${CC} ${CFLAGS} -c sort.c -o sort.o

display.o: display.c display.h filesystem.h options.h
	${CC} ${CFLAGS} -c display.c -o display.o

clean:
	rm -f ${OBJS} ${TARGET}

.PHONY: all clean
