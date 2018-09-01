#include <stdio.h> /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> /*文件控制定义*/
#include <termios.h> /*PPSIX 终端控制定义*/
#include <errno.h> /*错误号定义*/
#include <string.h>

#define FALSE -1
#define TRUE 0

#define BAUDRATE        115200
#define UART_DEVICE     "/dev/ttyUSB0"

typedef unsigned char  uint8_t;
typedef uint8_t  u8;

int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200,  300 };
void set_speed(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    //Opt.c_oflag  = ~ICANON;
    Opt.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    Opt.c_iflag &= ~(INLCR | ICRNL);
    Opt.c_iflag &= ~(IXON);    //关闭软件流控，解决了无法输出0x11、0x13的问题
    Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //Opt.c_iflag &- ~(ISTRIP);  //禁止将所有接受的字符裁剪为7比特
    //Opt.c_cflag &= ~(CRTSCTS);
    for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++ )
    {
        if  (speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);  //int tcflush（int filedes，int quene）
                                       // quene数该当是下列三个常数之一:
                                       //*TCIFLUSH  刷清输入队列
                                       //*TCOFLUSH  刷清输出队列
                                       //*TCIOFLUSH 刷清输入、输出队列
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);  //tcsetattr函数用于设置终端参数
                                                    //参数fd为打开的终端文件描述符，参数optional_actions用于控制修改起作用的时间，而结构体termios_p中保存了要修改的参数。
                                                    //TCSANOW：不等数据传输完毕就立即改变属性。
                                                    //TCSADRAIN：等待所有数据传输结束才改变属性。
                                                    //TCSAFLUSH：等待所有数据传输结束,清空输入输出缓冲区才改变属性。
            if  (status != 0) {
                perror("tcsetattr fd1");
                return;
            }
            tcflush(fd,TCIOFLUSH);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄
*@param  databits 类型  int 数据位   取值 为 7 或者8
*@param  stopbits 类型  int 停止位   取值为 1 或者2
*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_Parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if  ( tcgetattr( fd, &options )  !=  0) {   //tcgetattr函数用于获取与终端相关的参数
                                                //options该结构体一般包括如下的成员：
                                                 //tcflag_t c_iflag;
                                                 //tcflag_t c_oflag;
                                                 //tcflag_t c_cflag;
                                                 //tcflag_t c_lflag;
                                                 //cc_t c_cc[NCCS];
        perror("SetupSerial 1"); //perror()用来将上一个函数发生错误的原因输出到标准设备(stderr)。参数s所指的字符串会先打印出,后面再加上错误原因字符串。此错误原因依照全局变量errno的值来决定要输出的字符串。perror函数只是将你输入的一些信息和现在的errno所对应的错误一起输出。
        return(FALSE);
    }
    options.c_cflag &= ~CSIZE;   //???用数据位掩码清空数据位设置


    switch (databits) /*设置数据位数*/
    {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr,"Unsupported data size\n"); return (FALSE);
    }

    switch (parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB;   /* Clear parity enable */
            options.c_iflag &= ~INPCK;     /* Enable parity checking */
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
            options.c_iflag |= INPCK;             /* Disnable parity checking */
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;     /* Enable parity */
            options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
            options.c_iflag |= INPCK;       /* Disnable parity checking */
            break;
        case 'S':
        case 's':  /*as no parity*/
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;break;
        default:
            fprintf(stderr,"Unsupported parity\n");
            return (FALSE);
    }
    /* 设置停止位*/
    switch (stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return (FALSE);
    }
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;   /* Disnable parity checking */
    tcflush(fd,TCIFLUSH);
    options.c_cc[VTIME] = 0; /* 设置超时15 seconds*/
    options.c_cc[VMIN] = 14; /* Update the options and do it NOW */
    if (tcsetattr(fd, TCSANOW, &options) != 0)//tcsetattr函数用于设置终端参数。函数在成功的时候返回0，失败的时候返回-1，并设置errno的值。
    {
        perror("SetupSerial 3");
        return (FALSE);
    }
   // options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/

    //options.c_iflag &= ~(IXON | IXOFF | IXANY);
    //options.c_oflag &= ~OPOST;   /*Output*/
    //options.c_oflag &= ~(ONLCR | OCRNL);    //这；这两行处理发送0x0d会收到0x0a的情况
    return (TRUE);
}

int main(int argc, char *argv[])
{

    int fd, c=0, res, Z_Angle;
    u8 buf[12];
    printf("Start...\n");
    fd = open(UART_DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror(UART_DEVICE);
        exit(1);
    }
    printf("Open...\n");

    set_speed(fd, BAUDRATE);
    if (set_Parity(fd,8,1,'N') == FALSE)
    {
        printf("Set Parity Error\n");
        exit (0);
    }
    printf("Reading...\n");

    while(1)
    {
        res = read(fd, buf, 12);
        if(res > 0)
        {
            int i;
            if(res > 1) {
                printf("The Data is:");
                for (i = 0; i < res - 2; i++) {
                    printf("0x%02X ", buf[i]);
                    // if ((i+1)%10==0)
                    //   printf("\n");
                }
                //for (i = res - 2; i < res; i++) {
                Z_Angle = ((buf[res-2] * 256 + buf[res-1]) - 30000)/100;
                printf("%d\n ", Z_Angle);
               // }
                //printf("readlength = %d\n", res);
               // sleep(1);
               // tcflush(fd, TCIFLUSH); //清空缓存区
            }
        }
        //if (buf[0] == 0x0d) //回车
           // printf("11\n");
        //if (buf[0] == '@') break;
    }
    printf("Close...\n");
    close(fd);
    return 0;
}