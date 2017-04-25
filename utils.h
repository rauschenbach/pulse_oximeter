#ifndef _UTILS_H
#define _UTILS_H

#include "globdefs.h"

void add_crc16(u8 *buf);
void print_data_hex(u8*, int);
s64  get_msec_ticks(void);
s32  get_sec_ticks(void);

int is_end_thread(void);
void halt_all_threads(void);

#endif /* utils.h */


