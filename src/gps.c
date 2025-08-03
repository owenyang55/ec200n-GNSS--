#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "uart.h"
#include "gps.h"


//void QuecLocator(int fd);

//初始化GNSS配置
int GNSS_Init(int fd){
    printf("----Init GNSS----\n");

    //配置为GPS+BEIDOU
    if(strstr(set_recv_data(fd,"AT+QGPSCFG=\"gnssconfig\""),"5") == NULL){
        const char* ret = set_recv_data(fd,"AT+QGPSCFG=\"gnssconfig\",5");
        if(strstr(ret,"OK") != NULL){
            printf("configured for GPS+BEIDOU\n");
        }
        else{
            printf("config error\n");
            return -1;
        }
    }
    //打开GPS
    if(strstr(set_recv_data(fd,"AT+QGPS?"),"1") == NULL){
        set_recv_data(fd,"AT+QGPS=1");
        set_recv_data(fd,"AT+QGPS?");
        printf("GNSS ready\n");
    }    
    return 0;
}


/**
 * @brief 获取经纬度
 * @param fd 文件描述符
 * @param data 传出参数，负责接收经纬度
 * @return 成功0,失败-1
 */

int GNSS_data(int fd,struct send_data* data){
    
    //获取时间信息
    char buf[128] = {0};
    const char* origin_time = set_recv_data(fd,"AT+CCLK?");
    int ret = convert_time(origin_time,buf,sizeof(buf));
    if(ret == -1){
        printf("convert time failed\n");
        return -1;
    }
    printf("RECV time:%s\n",buf);
    

        
    const char* response;
    while(1){
        // 1. 直接使用 AT+QGPSLOC? 获取所有信息
        response = set_recv_data(fd, "AT+QGPSLOC?");

        // 2. 检查是否未定位 (最常见情况)
        if (strstr(response, "CME ERROR: 516")) {
            printf("GNSS failed\n");
            // printf("GNSS failed.Falling back to LBS/Wi-Fi positioning...\n");
            // QuecLocator(fd); 需付费开启
        }
        // 3. 检查响应头，确保是有效数据
        else if (strstr(response, "+QGPSLOC:") != NULL) {
            printf("GNSS succeed\n");
            break;
        }  

        sleep(5);   //每五秒尝试重新定位
    }

    // printf("%s\n",response);

    //预处理传出的字符串
    char* clean_res = strpbrk(response, "\r\n");
    if(clean_res != NULL){
        *clean_res = '\0';
    }

    //处理位置信息，使其只输出经纬度
    char utc_time[16] = {0}, lat_str[16] = {0}, lon_str[16] = {0}, date_str[8] = {0};
    char lat_dir = '\0', lon_dir = '\0';
    if (sscanf(clean_res, "%*[^:]: %15[^,],%15[^NnSs],%c,%15[^EeWw],%c,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%7[^,]",
               utc_time, lat_str, &lat_dir, lon_str, &lon_dir, date_str) < 6) {
        printf("Error: Failed to parse the GNSS response: %s\n", response);
        return -1;
    }
    double latitude = convert_long_and_lati(lat_str);
    double longtitude = convert_long_and_lati(lon_str);

    //传出前用正负区分半球
    if (lat_dir == 'S' || lat_dir == 's') {
        latitude = -latitude;
    }
    if (lon_dir == 'W' || lon_dir == 'w') {
        longtitude = -longtitude;
    }
    
    // 存储数据（注意：这里需要修正time的赋值）
    // data->time = buf; // 原写法错误，buf是局部数组
    strncpy(data->time, buf, sizeof(data->time) - 1);
    data->time[sizeof(data->time) - 1] = '\0';
    data->latitude = latitude;
    data->longtitude = longtitude;

    printf("[%s] 纬度：%.6f%c 经度：%.6f%c\n",buf,latitude,lat_dir,longtitude,lon_dir);
    return 0;
}


/**
 * @brief 将经纬度从NMEA标准修改为十进制度
 * @param nmea_str NMEA标准字符串
 * @return 返回转换后的浮点数
 */
double convert_long_and_lati(const char* nmea_str){
    double nmea_val = atof(nmea_str);
    double degrees = floor(nmea_val / 100);
    double minutes = fmod(nmea_val ,100);
    return degrees +(minutes / 60);
}



/**
 * @brief 将EC200N的CCLK时间字符串转换为本地时间字符串。
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


// /**
//  * @brief 辅助定位功能（使用LBS/Wifi辅助）废弃ing 付费开启功能通道
//  * @param fd 文件描述符
//  * @return 0 表示成功, -1 表示失败。
//  */
// void QuecLocator(int fd){
//     set_recv_data(fd,"AT+QLBS=1");
    
// }

//获取位置信息(需付费)
//set_recv_data(fd,"AT+QLBSCFG?");
// set_recv_data(fd,"AT+QLBS=2,\"94:B6:09:29:72:4B\",-30,\"1c:8b:ef:4c:5e:87\",-39");
// const char* locate = set_recv_data(fd,"AT+QLBS");
// printf("RECV location:%s\n",locate);