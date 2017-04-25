#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <termios.h>
#include <sys/select.h>
#include "globdefs.h"
#include "mythreads.h"
#include "comport.h"
#include "utils.h"


/* Num rates */
#define DEF_RATE	3

static struct {
    int sym;
    int data;
} rate[] = {
    { B2400, 2400},
    { B4800, 4800},	
    { B9600, 9600},
    { B19200, 19200},
    { B38400, 38400},
    { B57600, 57600},
    { B115200, 115200}, 
    { B230400, 230400}, 
    { B460800, 460800}, 
};

static int com_fd;


/**
 * Настраиваем порт 
 */
int com_port_open(char *name, u32 b)
{
    int i, baud = 0;
    struct termios oldtio, newtio;
    char buf[255];
    int res = -1;

    do {

	/* Найдем скорость  */
	for (i = 0; i < sizeof(rate)/sizeof(rate[0]); i++) {
	    if (rate[i].data == b) {
		baud = rate[i].sym;
		printf("INFO: baud rate set to %d\n", rate[i].data);
		break;
	    }
	}

	/* Если не нашли - поставим дефолтную */
	if (baud == 0) {
	    baud = rate[DEF_RATE].sym;
	    printf("WARN: set rate to default: %d\n", rate[DEF_RATE].data);
	}


	/* Открываем порт */
	com_fd = open(name, O_RDWR | O_NOCTTY | O_EXCL);
	if (com_fd < 0) {
	    perror("open");
	    break;
	}
	printf("INFO: port open OK\n");
	
	tcgetattr(com_fd, &oldtio);	/* сохранение текущих установок порта */
	bzero(&newtio, sizeof(newtio));

	/* Неканонический режим. Нет эхо. 8N2 */
	newtio.c_cflag |= (CLOCAL | CREAD);
	newtio.c_cflag &= ~PARENB;
	newtio.c_cflag |= CSTOPB; /* 2 stop */
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8;
	newtio.c_cflag &= ~CRTSCTS;

	newtio.c_lflag &= ~(ICANON | ECHO | ISIG);
#if 1
	newtio.c_cc[VTIME] = 1;	/* посимвольный таймер */
	newtio.c_cc[VMIN] = 0;
#else
	newtio.c_cc[VTIME] = 0;	/* посимвольный таймер */
	newtio.c_cc[VMIN] = 1;
#endif
	newtio.c_oflag &= ~OPOST;
	newtio.c_iflag |= IGNPAR;


	cfsetispeed(&newtio, baud);
	cfsetospeed(&newtio, baud);


	/* Noblock  */
	if (tcsetattr(com_fd, TCSANOW, &newtio) < 0) {
	    perror("tcsetatr");
	    res = false;
	    break;
	}

        /* Неблокирующий режим */
	i = fcntl(com_fd, F_GETFL, 0);
	fcntl(com_fd, F_SETFL, i | O_NONBLOCK);


	/* Заблокировать для других процессов */
	ioctl(com_fd, TIOCEXCL);

	res = 0;
        sprintf(buf, "INFO: open port %s on baud %d", name, b);
    } while (0);


    return res;
}

/**
 * Закрыть порт 
 */
int com_port_close(void)
{
    /* Сбросим буферы */
    tcflush(com_fd, TCIFLUSH);
    tcflush(com_fd, TCOFLUSH);
    ioctl(com_fd, TIOCNXCL);
    close(com_fd);
    return 0;
}

/**
 * Сброс буферов  
 */
void com_port_reset(void)
{
    tcflush(com_fd, TCIFLUSH);
    tcflush(com_fd, TCOFLUSH);
}


/**
 * Запись в порт
 */
int com_port_write(u8 *buf, int len)
{
    int num = -1;

    if (com_fd > 0) {
	num = write(com_fd, buf, len);
    } 
    return num;
}


/**
 * Чтение из порта без таймаута
 */
int com_port_read(u8 *buf, int len)
{
    int num = 0, res, i = 0;
    u8 *ptr;
    fd_set readfs;		/* file descriptor set */
    int maxfd;			/* maximum file desciptor used */
    struct timeval timeout;
    int ind = 0;

    if (com_fd > 0) {

	ptr = buf;
	while (1) {
	    FD_ZERO(&readfs);
	    FD_SET(com_fd, &readfs);	/* set testing for source 1 */
	    maxfd = com_fd + 1;	/* maximum bit entry (fd) to test */

	    timeout.tv_sec = 0;	/* секунды */
	    timeout.tv_usec = 25000;	/* микросекунды */
	    res = select(maxfd, &readfs, NULL, NULL, &timeout);

	    if (FD_ISSET(com_fd, &readfs)) {
		num = read(com_fd, ptr, len - ind);

		if(num <= 0) {
		    if(num < 0) {
			printf("ERROR: num < 0!\n");	    
		    } else {
			printf("nothing to do!\n");
		    }
		    break;
		}

		ptr += num;
		ind += num;
		if(is_end_thread) {
		    break;
		}
		
		if (ind >= len || num < 0) {
		    if(num < 0){
			printf("ERROR: read from port\n");
			}
		    break;
		}
	    } else {
		break;
	    }
	}
    }
//    tcflush(com_fd, TCIFLUSH);
//    tcflush(com_fd, TCOFLUSH);

    return ind;
}
