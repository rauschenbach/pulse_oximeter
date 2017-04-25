#define _BSD_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <termios.h>
#include <byteswap.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "mythreads.h"
#include "comport.h"
#include "circbuf.h"
#include "utils.h"



#define	UDP_PORT	1025

#define CIRC_BUF_SIZE	512
#define READ_BUF_SIZE	128
#define BAUD_RATE	19200


/* Статические пременные */

static char com_name[32];
static int com_present = 0;
static int net_fd;		/* Сокет UDP (пока!) */
static struct sockaddr_in sa;	/* переменная адреса интерфейса */
static CircularBuffer cb;


/* Поточные функции */
static void *find_port_func(void *);
static void *read_port_func(void *);
static void *write_net_func(void *);


static pthread_t find_port_thread;	/* Чтение из порта */
static pthread_t read_port_thread;	/* Чтение из порта */
static pthread_t write_net_thread;	/* Исполнение команды по сетке */


/* Создадим сокет приема и передачи */
int open_net_channel(void)
{
    bool res = -1;
    int port = UDP_PORT;

    do {

	/* Создадим датаграмный сокет UDP */
	if ((net_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	    perror("socket");
	    break;
	}

	/* Обнуляем переменную m_addr и забиваем её нужными значениями */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;	/* обязательно AF_INET! */
	sa.sin_addr.s_addr = htonl(INADDR_ANY);	/* C любого интерфейса принимаем */
	sa.sin_port = htons(port);	/* 0 - выдать порт автоматом */

	/* привяжем сокет */
	if (bind(net_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	    perror("bind");
	    close(net_fd);
	    break;
	}
	res = 0;
    } while (0);
    return res;
}


/**
 * Закрыть сетевой канал 
 */
void close_net_channel(void)
{
    shutdown(net_fd, SHUT_RD);
    close(net_fd);
}


/** 
 * Запуск всех потоков  
 */
int run_all_threads(void)
{
    int i;
    int res = EXIT_FAILURE;

    do {

	/* Создадим кольцевой буфер */
	if (cb_init(&cb, CIRC_BUF_SIZE)) {
	    printf("ERROR: can't create circular buffer\n");
	    break;
	}
	printf("INFO: circular buffer init OK\n");

	/* Создаем сетевой сокет  */
	if (open_net_channel()) {
	    printf("ERROR: create net channel\n");
	    break;
	}
	printf("INFO: create net channel OK\n");
#if 1
	/* Создаем поток для работы по сети  */
	if (pthread_create(&write_net_thread, NULL, write_net_func, NULL) != 0) {
	    printf("ERROR: create net thread\n");
	    break;
	}
#endif
	/* Создаем поток для поиска USB порта  */
	if (pthread_create(&find_port_thread, NULL, find_port_func, NULL) != 0) {
	    printf("ERROR: create net thread\n");
	    break;
	}

	/* Создаем поток для чтения из порта */
	if (pthread_create(&read_port_thread, NULL, read_port_func, NULL) != 0) {
	    printf("ERROR: create port thread\n");
	    break;
	}
#if 1
	if (pthread_join(write_net_thread, NULL) != 0) {
	    printf("ERROR: net pthread_join\n");
	    break;
	}
#endif
	if (pthread_join(find_port_thread, NULL) != 0) {
	    printf("ERROR: find port pthread_join\n");
	    break;
	}

	if (pthread_join(read_port_thread, NULL) != 0) {
	    printf("ERROR: read port pthread_join\n");
	    break;
	}
	res = EXIT_SUCCESS;
    } while (0);
    printf("SUCCESS: thread exit\n");
    return res;
}


/**
 * Потоковая функция передачи и приема по сети
 */
static void *write_net_func(void *par)
{
    int n, i = 0, num;
    u32 fromlen;
    port_cmd_enum cmd;
    struct hostent *ht;
    char buf_rx[32];
    char buf_tx[READ_BUF_SIZE];
    fromlen = sizeof(sa);


    /* Считываем из кругового буфера и передаем обратно */
    while (!is_end_thread()) {
//      printf("recvfrom:\n ");

	n = recvfrom(net_fd, (void *) buf_rx, sizeof(buf_rx), 0, (struct sockaddr *) &sa, &fromlen);
	if (n < 0) {
	    perror("recfrom");
	    break;

	} else {
	    memcpy(&cmd, buf_rx, 4);
	    switch (cmd) {
	    case cmd_open:
		printf("recv: open port\n");
		break;

	    case cmd_close:
		printf("recv: close port\n");
		break;

	    case cmd_write:
		printf("recv: write port\n");
		break;

		/* читаем из буфера и передаем */
	    case cmd_read:
		memcpy(&num, buf_rx + 4, 4);
		i = 0;
		while(!cb_is_empty(&cb) && i < num) {
		    cb_read(&cb, &buf_tx[i++]);
		} 
		printf("recv: read port. sendto %d bytes\n", i);
		n = sendto(net_fd, buf_tx, i, 0, (struct sockaddr *) &sa, fromlen);
		break;

	    default:
		printf("recv: unknown cmd\n");
		break;
	    }

	    print_data_hex(buf_tx, i);
	}


#if 0
	switch (cmd[0]) {
	case cmd_read:
	    u32 rd;
	    /* 1 4 байта команда, далее сколько прочитать */
/*	    memcpy(&rd, cmd + 4, 4);

	    // читаем из буфера и передаем           
	    for (i = 0; i < rd; i++) {
		cb_read(&cb, &buf[i]);
	    }
*/
//          n = sendto(net_fd, wr_buf, sizeof(status), 0, (struct sockaddr *) &sa, fromlen);

	    print_data_hex(cmd, n);

	    break;

	default:
	    break;
	}
#endif
    }

    printf("INFO: read net sock pthread end\n");
    pthread_exit(EXIT_SUCCESS);
}

/**
 * Чтение / запись в порт 
 */
static void *read_port_func(void *par)
{
    char buf[READ_BUF_SIZE];
    int i;


    /* Дожидаемся порта */
    while (!com_present && !is_end_thread());

    printf("INFO: read port thread begin\n");
    com_port_open(com_name, BAUD_RATE);

    /* Читаем данные если запущено */
    while (!is_end_thread()) {

	if (com_port_read(buf, READ_BUF_SIZE)) {

	    /* в кольцевой буфер до половины.
	     * успеет записаться за время между 2-мя символами */
	    for (i = 0; i < READ_BUF_SIZE; i++) {
		cb_write(&cb, &buf[i]);
	    }
//          print_data_hex(buf, READ_BUF_SIZE);
	}
    }
    printf("INFO: read port thread end\n");
    pthread_exit(EXIT_SUCCESS);
}


/**
 * Найти свободный USB порт
 */
static void *find_port_func(void *par)
{
    int fd, i;
    char str[32];

    /* Пытаемся открыть и проверить на доступ порт */
    for (i = 0; i < 255; i++) {
	sprintf(com_name, "/dev/ttyUSB%i", i);
	fd = open(com_name, O_RDWR | O_NOCTTY);
	if (fd < 0) {
	    sprintf(str, "open %s", com_name);
	    perror(str);
	    continue;
	} else {
	    /* Закроем порт  */
	    close(fd);
	    printf("INFO: %s is free port\n", com_name);
	    com_present = 1;
	    break;
	}
    }
    printf("INFO: find port thread end\n");
    pthread_exit(EXIT_SUCCESS);
}
