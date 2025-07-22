#ifndef __UART_H__
#define __UART_H__

extern int set_uart(int fd, int speed, int bits, char check, int stop);
extern const char* set_recv_data(int fd,const char *data);
int ec200_init(int fd);

#endif