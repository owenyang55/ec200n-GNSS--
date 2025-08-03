#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include "uart.h"
#include <termios.h>

#define CON_PLATFORM "onenet"
#define CON_PRODUCTID "37Vi53L8K3"
#define CON_ACCESSKEY "Y0xWSmdOSnRBUlM3RWVidXlpazF2N3BSQUs4U0FOM24="
#define CON_WEB "mqtts.heclouds.com"
#define CON_PORT 1883
#define CON_CLIENTID "4g_module"
#define CON_USERNAME "37Vi53L8K3"
#define CON_PASSWORD "Y0xWSmdOSnRBUlM3RWVidXlpazF2N3BSQUs4U0FOM24="
/**
 * @brief 初始化SIM卡
 * @param fd 文件描述符
 * @return 成功0,失败-1
 */
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
        if((strstr(ret,",1")) != NULL || (strstr(ret,",5")) != NULL){
            printf("Network is ready\n");
            break;
        }
        usleep(100000);
    }

    if(count < 0){
        printf("Network failed\n");
        return -1;
    }
    
    //4、关闭之前的TCP连接进行初始化
    ret = set_recv_data(fd,"AT+QICLOSE=1");
    if(strstr(ret,"OK") != NULL){
        printf("Socket is closed.\n");
    }

    return 0;
}


/**
 * @brief 初始化TCP连接
 * @param fd 文件描述符
 * @param APN 运营商代码
 * @param server_ip 服务器ip
 * @return 成功0,失败-1
 */
int socket_init(int fd, const char* APN, const char* server_ip){

    //1、格式化字符串并配置TCP场景
    char command[128];
    const char* format_sgp = "AT+QICSGP=1,1,\"%s\","","",1";
    int len = snprintf(command,sizeof(command),format_sgp,APN);
    if(len < 0 || len >=sizeof(command)){
        printf("Error:Failed to format qic_command\n");
        return -1;
    }

    const char* ret = set_recv_data(fd,command);
    if(strstr(ret,"OK") != NULL)
        printf("QICSGP succeed\n");

    //2、激活场景1
    ret = set_recv_data(fd,"AT+QIACT?"); //查看当前是否已有场景
    if(strstr(ret,"+QIACT") != NULL){
        printf("QIACT succeed\n");
    }
    else{
        ret = set_recv_data(fd,"AT+QIACT=1");
        if(strstr(ret,"OK") != NULL)
        printf("QIACT succeed\n");
    }
    
        
    

    //3、打开TCP连接

        // ---------------------------------------------------------------------
    // 步骤3：打开TCP连接 (简洁、健壮的替换方案)
    // ---------------------------------------------------------------------
    // char command[128];
    const char* format_tcp = "AT+QIOPEN=1,0,\"TCP\",\"%s\",9999,0,0";
    snprintf(command, sizeof(command), format_tcp, server_ip);

    // a. 发送指令
    char write_buf[128];
    snprintf(write_buf, sizeof(write_buf), "%s\r\n", command);
    printf("==> Sending connect command: %s", write_buf);
    
    // 发送前清空输入缓冲区，防止读到旧数据
    tcflush(fd, TCIFLUSH); 
    if (write(fd, write_buf, strlen(write_buf)) < 0) {
        perror("write for QIOPEN failed");
        return -1;
    }

    // b. 循环等待，直到成功、失败或超时
    char response_buffer[512] = {0}; // 用于拼接所有收到的数据
    int total_timeout_ms = 30000;    // 总超时时间：30秒
    int step_timeout_ms = 500;       // 每次读取的等待间隔：0.5秒
    int time_waited_ms = 0;

    while (time_waited_ms < total_timeout_ms) {
        char chunk[128] = {0};
        int read_len = read(fd, chunk, sizeof(chunk) - 1);
        
        if (read_len > 0) {
            // 将新读到的数据拼接到总缓冲区
            strncat(response_buffer, chunk, sizeof(response_buffer) - strlen(response_buffer) - 1);
            printf("<== Received chunk: %s\n", chunk);

            // 检查是否同时包含了两个成功标志
            if (strstr(response_buffer, "OK") && strstr(response_buffer, "+QIOPEN: 0,0")) {
                printf("QIOPEN succeed. Connection confirmed.\n");
                return 0; // *** 这是唯一的成功出口 ***
            }
            // 检查是否收到了明确的失败信息
            if (strstr(response_buffer, "ERROR") || (strstr(response_buffer, "+QIOPEN: 0,") && !strstr(response_buffer, "+QIOPEN: 0,0"))) {
                printf("Error: QIOPEN command failed explicitly.\nResponse: %s\n", response_buffer);
                return -1;
            }
        }
        
        // 等待一小段时间再试
        usleep(step_timeout_ms * 1000);
        time_waited_ms += step_timeout_ms;
    }

    // c. 如果循环结束还没成功，就是超时了
    printf("Error: Timeout. Failed to get confirmation for QIOPEN within %d seconds.\n", total_timeout_ms / 1000);
    return -1;
} 

// socket_init 函数到此结束
    // const char* format_tcp = "AT+QIOPEN=1,0,\"TCP\",\"%s\",9999,0,0";
    // len = snprintf(command,sizeof(command),format_tcp,server_ip);
    // if(len < 0 || len >=sizeof(command)){
    //     printf("Error:Failed to format tcp_command\n");
    //     return -1;
    // }

    // ret = set_recv_data(fd,command);
    // if(strstr(ret,"OK") != NULL)
    //     printf("QIOPEN succeed\n");
    // return 0;
// }


/**
 * @brief 向TCP发送数据
 * @param fd 文件描述符
 * @param connectID 连接ID，与open中的一致
 * @param data 输入的命令
 * @return 成功0,失败-1
 */
int send_tcpdata(int fd, int connectID, const char* data){
    char command_buffer[32];
    char recv_buffer[512];
    size_t data_len = strlen(data);

    // 在进入特殊流程前，清空一下串口可能残留的旧数据
    tcflush(fd, TCIFLUSH);

    // --- 步骤 1: 发送 AT+QISEND=<connectID> 指令 ---
    snprintf(command_buffer, sizeof(command_buffer), "AT+QISEND=%d\r\n", connectID);
    printf("==> Step 1: Sending command: %s", command_buffer);
    if (write(fd, command_buffer, strlen(command_buffer)) < 0) {
        perror("write failed for QISEND command");
        return -1;
    }
    tcdrain(fd); // 等待数据发送完成

    // --- 步骤 2: 等待 '>' 提示符 (使用 poll 实现带超时的读取) ---
    printf("==> Step 2: Waiting for '>' prompt...\n");
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int poll_ret = poll(&pfd, 1, 3000); // 3秒超时

    if (poll_ret < 0) {
        perror("poll failed");
        return -1;
    }
    if (poll_ret == 0) {
        printf("Error: Timeout. Did not receive '>' prompt within 3 seconds.\n");
        return -1;
    }

    // 确认有数据可读后，再读取
    memset(recv_buffer, 0, sizeof(recv_buffer));
    int read_len = read(fd, recv_buffer, sizeof(recv_buffer) - 1);
    if (read_len <= 0) {
        perror("read failed after poll");
        return -1;
    }
    
    if (strstr(recv_buffer, ">") == NULL) {
        printf("Error: Did not receive '>' prompt. Module response was: %s\n", recv_buffer);
        return -1;
    }
    printf("==> Received '>' prompt. Ready to send data.\n");

    // --- 步骤 3: 发送数据负载和结束符 ---
    printf("==> Step 3: Sending data payload and Ctrl+Z...\n");
    // 发送数据
    if (write(fd, data, data_len) < 0) {
        perror("write failed for data payload");
        return -1;
    }
    // 发送 Ctrl+Z (0x1A)
    const char ctrl_z = 0x1A;
    if (write(fd, &ctrl_z, 1) < 0) {
        perror("write failed for Ctrl+Z");
        return -1;
    }
    tcdrain(fd); // 等待发送完成

    // --- 步骤 4: 等待最终响应 (SEND OK / SEND FAIL / ERROR) ---
    printf("==> Step 4: Waiting for final confirmation...\n");
    poll_ret = poll(&pfd, 1, 10000); // 10秒长超时等待网络发送

    if (poll_ret <= 0) {
        printf("Error: Timeout or poll error while waiting for 'SEND OK'.\n");
        return -1;
    }

    memset(recv_buffer, 0, sizeof(recv_buffer));
    read_len = read(fd, recv_buffer, sizeof(recv_buffer) - 1);
    if (read_len <= 0) {
        perror("read failed while waiting for final confirmation");
        return -1;
    }
    recv_buffer[read_len] = '\0';
    printf("<== Final response: \n---\n%s\n---\n", recv_buffer);

    if (strstr(recv_buffer, "SEND OK") != NULL) {
        printf("Success: Data sent successfully.\n");
        return 0;
    } else {
        printf("Failure: Did not receive 'SEND OK'.\n");
        return -1;
    }
}



/**
 * @brief 初始化MQTT
 * @param fd 文件描述符
 * @return 成功0,失败-1
 */
int mqtt_init(int fd){
    //1、配置MQTT模式，将版本设置为3.1.1
    //client_id为0
    char command[128];
    const char* ret = set_recv_data(fd,"AT+QMTCFG=\"version\",0,4"); 
    if(strstr(ret,"OK") == NULL){
        printf("error in set version\n");
        return -1;
    }

    //配置接收模式
    ret = set_recv_data(fd,"AT+QMTCFG=\"recv/mode\",0,0,1");
    if(strstr(ret,"OK") == NULL){
        printf("error in set recv\n");
        return -1;
    }

    //配置onenet平台设备信息
    memset(command, 0, sizeof(command)); 
    sprintf(command,"AT+QMTCFG=%s,0,%s,%s",CON_PLATFORM,CON_PRODUCTID,CON_ACCESSKEY);
    ret = set_recv_data(fd,command);
    if(strstr(ret,"OK") == NULL){
        printf("error in cfg onenet\n");
        return -1;
    }

    return 0;
}


/**
 * @brief 打开并连接服务器
 * @param fd 文件描述符
 * @return 成功0,失败-1
 */
int mqtt_connect(int fd){
    char command[128];
    //打开服务器
    memset(command, 0, sizeof(command)); 
    sprintf(command,"AT+QMTOPEN=0,%s,%d",CON_WEB,CON_PORT);
    const char* ret = set_recv_data(fd,command);
    if(strstr(ret,"OK") == NULL){
        printf("error in onenet open\n");
        return -1;
    }  
    set_recv_data(fd,"AT+QMTOPEN?");
        
    //连接到服务器
    memset(command, 0, sizeof(command)); 
    sprintf(command,"AT+QMTCONN=0,%s,%s,%s",CON_CLIENTID,CON_USERNAME,CON_PASSWORD);
    ret = set_recv_data(fd,command);
        if(strstr(ret,"OK") == NULL){
        printf("error in onenet connection\n");
        return -1;
    }  
    return 0;
}

/**
 * @brief 断开服务器
 * @param fd 文件描述符
 * @return 成功0,失败-1
 */
int mqtt_close(int fd){
    const char* ret = set_recv_data(fd,"AT+QMTDISC=0");
    if(strstr(ret,"OK") == NULL){
        printf("error in onenet closed\n");
        return -1;
    }  
    return 0;
}


/**
 * @brief 发送数据
 * @param fd 文件描述符
 * @param connectID 连接ID，与open中的一致
 * @param data 输入的命令
 * @return 成功0,失败-1
 */
int mqtt_send_data(int fd, const char* time, double latitude, double longtitude){
    char command[128];
    char tmp[128];
    memset(command, 0, sizeof(command)); 
    memset(tmp, 0, sizeof(tmp)); 
    //订阅物模型属性类相关topic
    sprintf(command,"AT+QMTPUBEX=0,0,0,0\"$sys/%s/%s/thing/property/post\",128",CON_PRODUCTID,CON_CLIENTID);
    const char* ret = set_recv_data(fd,command);
    if(strstr(ret,">") == NULL){
        printf("error in subscribing\n");
        return -1;
    }
    //构造JSON数据进行传输
    memset(command, 0, sizeof(command)); 
    sprintf(command,"{\"id\":\"123\",\"params\":{\"time\":{\"value\":%s},\"location\":{\"value\":%lf,%lf}}}",time,latitude,longtitude);
    ret = set_recv_data(fd,command);
    if(strstr(ret,"OK") == NULL){
        printf("error in sending\n");
        return -1;
    }

    return 0;
}









// int send_tcpdata(int fd,const char* data){

//     set_recv_data(fd,"AT+QISEND=0");

//     const char* ret = set_recv_data(fd,data);
//     if(strstr(ret,"OK") != NULL)
//         printf("data send\n");
//     else if(strstr(ret,"ERROR") != NULL){
//         printf("Error in data send\n");
//         return -1;
//     }
        
//     return 0;
// }


















// /**
//  * @brief 初始化MQTT
//  * @param fd 文件描述符
//  * @return 成功0,失败-1
//  */
// int mqtt_init(int fd){
//     const char* ret = set_recv_data(fd,"AT+QMTCFG=\"recv/mode\",0,0,1");
//     if(strstr(ret,"OK") != NULL){
//         printf("mqtt ok\n");
//     }
// }
