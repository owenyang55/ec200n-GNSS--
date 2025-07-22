#ifndef __GPS_H__
#define __GPS_H__

int GNSS_Init(int fd);
void GNSS_data(int fd);
// int convert_time(const char* cclk_response, char* buffer, size_t size);

#endif