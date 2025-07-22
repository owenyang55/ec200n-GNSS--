#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "uart.h"
#include "gps.h"
#include "sim.h"

int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     fprintf(stderr, "用法: %s \"AT指令\"\n", argv[0]);
    //     return 1;
    // }

    int fd, count;
    char command_buffer[128];

    //确定串口位置
    fd = open("/dev/ttyUSB1", O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open /dev/ttyUSB1");
        exit(-1);
    }

    //初始化串口
    if (set_uart(fd, 115200, 8, '0', 1) != 0) {
        close(fd);
        exit(-1);
    }


    //初始化ec200模块
    int flag = ec200_init(fd);
    if(flag == -1){
        close(fd);
        printf("error\n");
        exit(-1);
    }



    //初始化上网
    flag = sim_init(fd);
    if(flag == -1){
        close(fd);
        printf("error\n");
        exit(-1);
    }

    // 初始化GNSS
    flag = GNSS_Init(fd);
    if(flag == -1){
        close(fd);
        printf("error\n");
        exit(-1);
    }

    
    close(fd);
    return 0;
}