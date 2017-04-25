#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mythreads.h"
#include "comport.h"
#include "utils.h"

#define BAUD_RATE	19200

static void sig_handler(int num)
{
    printf("signal SIGINT!\n");
    halt_all_threads();
}

int main(int argc, char *argv[])
{
    int res;

    signal(SIGINT, sig_handler);

    run_all_threads();
    com_port_close();
    printf("INFO: stop device\n");
    return 0;
}
