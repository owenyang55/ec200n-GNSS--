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


// #define APN "CTNET" //电信
// #define IP "127.0.0.1" //虚拟机IP


int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     fprintf(stderr, "用法: %s \"AT指令\"\n", argv[0]);
    //     return 1;
    // }

    int fd, count;
    char command_buffer[128];
    //实例化结构体(位于gps.h)
    struct send_data data;

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
        printf("error in ec200_init\n");
        exit(-1);
    }



    //初始化上网
    flag = sim_init(fd);
    if(flag == -1){
        close(fd);
        printf("error in sim_init\n");
        exit(-1);
    }

    // 初始化GNSS
    flag = GNSS_Init(fd);
    if(flag == -1){
        close(fd);
        printf("error in gps_init\n");
        exit(-1);
    }

    //初始化mqtt协议
    flag = mqtt_init(fd);
    if(flag == -1){
        close(fd);
        printf("error in mqtt_init\n");
        exit(-1);
    }
    //连接物联网服务平台
    flag = mqtt_connect(fd);
    if(flag == -1){
        close(fd);
        printf("error in mqtt_connect\n");
        exit(-1);
    }

    sleep(1);

    //发送测试信息(2025/8/2)
    flag = mqtt_send_data(fd,"2001/12/21",60,120);

    //间隔30s循环打印时间及位置信息，并上传到服务器
    while(1){
        flag = GNSS_data(fd,&data);
        if(flag == -1){
            close(fd);
            printf("error in gps_data\n");
            exit(-1);
        }
        flag = mqtt_send_data(fd,data.time,data.latitude,data.longtitude);
        if(flag == -1){
            close(fd);
            printf("error in mqtt_send_data\n");
            exit(-1);
        }
        sleep(30);

    }

    mqtt_close(fd);
    close(fd);
    return 0;
}