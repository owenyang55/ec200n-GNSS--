#ifndef __GPS_H__
#define __GPS_H__

//定义
struct send_data
{
    char time[128];
    double latitude;
    double longtitude;
};

int GNSS_Init(int fd);
int GNSS_data(int fd,struct send_data* data);
int convert_time(const char* cclk_response, char* buffer, size_t size);
double convert_long_and_lati(const char* nmea_str);
// int convert_time(const char* cclk_response, char* buffer, size_t size);



#endif