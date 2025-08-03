#ifndef __SIM_H__
#define __SIM_H__

int sim_init(int fd);
int socket_init(int fd, const char* APN, const char* server_ip);
int send_tcpdata(int fd, int connectID, const char* data);
int mqtt_init(int fd);
int mqtt_connect(int fd);
int mqtt_close(int fd);
int mqtt_send_data(int fd, const char* time, double latitude, double longtitude);
#endif