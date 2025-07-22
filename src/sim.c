#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "uart.h"

int sim_init(int fd){
    //1、初始化sim卡
    const char* ret = set_recv_data(fd,"AT+CPIN?");
    if(strstr(ret,"READY") != NULL){
        printf("SIM card is ready.\n");
    }
    else{
        printf("ERROR: SIM card not ready or not detected.\n");
        return -1;
    }

    //2、检查信号质量
    int count = 10;
    while (count-- > 0)
    {
        ret = set_recv_data(fd,"AT+CSQ");
        //printf("%s",ret);
        if(strstr(ret,"99 99") == NULL){
            printf("Signal is ready\n");
            break;
        }
        usleep(100000);
    }
    if(count < 0){
        printf("Signal failed\n");
        return -1;
    }
    
    //3、检查网络注册状态
    count = 10;
    while (count-- > 0)
    {
        ret = set_recv_data(fd,"AT+CREG?");
        if((strstr(ret,",1")) != NULL | (strstr(ret,",5")) != NULL){
            printf("Network is ready\n");
            break;
        }
        usleep(100000);
    }

    if(count < 0){
        printf("Network failed\n");
        return -1;
    }
    
    return 0;

}