#include <stdio.h>
#include <time.h>
#include <string.h>
#include "uart.h"


int GNSS_Init(int fd);
void GNSS_data(int fd);
int convert_time(const char* cclk_response, char* buffer, size_t size);


//初始化GNSS配置
int GNSS_Init(int fd){
    printf("----Init GNSS----\n");

    //配置为GPS+BEIDOU
    const char* ret = set_recv_data(fd,"AT+QGPSCFG=\"gnssconfig\",5");
    if(strstr(ret,"OK") != NULL){
        printf("configured for GPS+BEIDOU\n");
    }
    else{
        printf("config error\n");
        return -1;
    }

    if(strstr(set_recv_data(fd,"AT+GPS=1"),"OK") != NULL){
        printf("GNSS ready\n");
    }
    else{
        printf("GNSS error\n");
        return -1;       
    }

    return 0;
}


//使用GPS功能
void GNSS_data(int fd){
    
    //获取时间信息
    char buf[128] = {0};
    const char* origin_time = set_recv_data(fd,"AT+CCLK?");
    int ret = convert_time(origin_time,buf,sizeof(buf));
    printf("RECV time:%s\n",buf);
    
    //获取位置信息
    //set_recv_data(fd,"AT+QLBSCFG?");
    // set_recv_data(fd,"AT+QLBS=2,\"94:B6:09:29:72:4B\",-30,\"1c:8b:ef:4c:5e:87\",-39");
    // const char* locate = set_recv_data(fd,"AT+QLBS");
    // printf("RECV location:%s\n",locate);
}




/**
 * @brief (简化版) 将EC200N的CCLK时间字符串转换为本地时间字符串。
 * 
 * @param cclk_response 模块返回的CCLK响应，如 "+CCLK: \"25/07/22,08:01:11+32\""。
 * @param buffer 用于存储结果的缓冲区。
 * @param size 缓冲区大小。
 * @return 0 表示成功, -1 表示失败。
 */
int convert_time(const char* cclk_response, char* buffer, size_t size) {
    struct tm time_info = {0};
    int year, month, day, hour, min, sec, tz_offset;

    // 1. 使用 sscanf 一次性解析出所有数字部分
    if (sscanf(cclk_response, "%*[^\"]\"%d/%d/%d,%d:%d:%d%d\"",
               &year, &month, &day, &hour, &min, &sec, &tz_offset) != 7) {
        return -1; // 解析失败
    }

    // 2. 填充tm结构体，并直接加上时区偏移（单位：分钟）
    time_info.tm_year = year + 100;     // 年份: 从1900年起
    time_info.tm_mon = month - 1;       // 月份: 0-11
    time_info.tm_mday = day;
    time_info.tm_hour = hour;
    time_info.tm_sec = sec;
    time_info.tm_min = min + (tz_offset * 15); // 直接加上时区分钟数

    // 3. 使用 mktime() 自动处理时间进位 (例如分钟超过60，小时超过24等)
    mktime(&time_info);

    // 4. 使用 strftime() 格式化输出最终结果
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &time_info);

    return 0; // 成功
}