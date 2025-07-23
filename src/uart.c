#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>




/**
 * @brief 串口收发，将信息发回
 * @param fd 文件描述符
 * @param data 串口输入的命令
 * @return 返回转换后的浮点数
 */
const char * set_recv_data(int fd,const char *data){
    char write_data[128];
    snprintf(write_data, sizeof(write_data), "%s\r", data); //加入\r符合AT指令要求
    printf("发送数据: %s", write_data); // \r 会使光标回到行首，所以打印可能看起来有点奇怪

    int ret = write(fd,write_data,strlen(write_data));
    if(ret == -1){
        perror("write");
        return "-1";
    }
    //等待响应结束
    tcdrain(fd);

    static char buff[128];
    ret = read(fd,buff,sizeof(buff) - 1);
    if (ret > 0) {
        buff[ret] = '\0';
        printf("\n收到响应 (%d 字节):\n---%s\n---\n", ret, buff);
    } 
    else if (ret == 0) {
        printf("\n在超时时间内没有收到数据。\n");}
    else {
        perror("\nread");
    }
    return buff;

}

//初始化ec200
int ec200_init(int fd){
    const char* buf = set_recv_data(fd,"AT");
    if(buf =="OK"){
        return 0;
    }
    else if(buf == "ERROR"){
        return -1;
    }
    set_recv_data(fd,"AT+QLBSCFG=\"token\"");
}
//初始化串口
int set_uart(int fd, int speed, int bits, char check, int stop) {
    struct termios newtio;

    if (tcgetattr(fd, &newtio) != 0) {
        perror("tcgetattr");
        return -1;
    }

    // 设置为原始模式
    cfmakeraw(&newtio);

    // 设置波特率
    speed_t baud_rate;
    switch (speed) {
        case 9600:   baud_rate = B9600;   break;
        case 115200: baud_rate = B115200; break;
        default:
            fprintf(stderr, "错误: 不支持的波特率 %d\n", speed);
            return -1;
    }
    cfsetispeed(&newtio, baud_rate);
    cfsetospeed(&newtio, baud_rate);

    // 数据位、校验位、停止位
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;       // 8 data bits
    newtio.c_cflag &= ~PARENB;   // No parity
    newtio.c_cflag &= ~CSTOPB;   // 1 stop bit

    // 禁用硬件和软件流控
    //newtio.c_cflag &= ~CRTSCTS;
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

    // 设置读取超时 (1秒超时)
    newtio.c_cc[VTIME] = 10;
    newtio.c_cc[VMIN]  = 0;

    // 清空缓冲区并应用设置
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &newtio) != 0) {
        perror("tcsetattr");
        return -1;
    }

    printf("串口配置成功: %d 8N1\n", speed);
    return 0;
}