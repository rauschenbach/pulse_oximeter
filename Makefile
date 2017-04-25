PROGNAME = sender
CC = gcc



CFLAGS = -o0 -ggdb3 ${CMP_OPTS} -DTEST_NMEA_TIME
###CFLAGS = -oS
LDFLAG = -D_REENTERANT -lpthread 

OBJS = circbuf.o comport.o utils.o mythreads.o sender.o
C_FILES = circbuf.c comport.o utils.c mythreads.c sender.c

TMPFILES =  *.c~ *.h~

all: ${PROGNAME}

${PROGNAME}: ${OBJS} Makefile
	${CC} -o ${PROGNAME} ${OBJS} ${LDFLAG}

clean:
	@rm -f $(subst /,\,${OBJS}) ${PROGNAME} *.*~

%.o: %.c 
	${CC} ${CFLAGS} -c $^ -o $@


.PHONY : clean
