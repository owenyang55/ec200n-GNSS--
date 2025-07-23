#ifndef __GPS_H__
#define __GPS_H__

int GNSS_Init(int fd);
int GNSS_data(int fd);
int convert_time(const char* cclk_response, char* buffer, size_t size);
double convert_long_and_lati(const char* nmea_str);
// int convert_time(const char* cclk_response, char* buffer, size_t size);

#endif