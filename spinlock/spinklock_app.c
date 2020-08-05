#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/*
 * 使用方法: ./spinlock_app /dev/driver_case num
 * num:      1: 传输1024个a字符     2: 传输1024个b字符
 */

static char usrdata_1[1024] = {0};
static char usrdata_2[1024] = {0};
static int usr_flag = 0;

int main(int argc, char *argv[])
{
    int fd, retvalue, i = 0;
    char *filename;
    char readbuf[1024], writebuf[1024];

    printf("argc: %d\n", argc);
	
    for(i = 0; i < 1024; i++) {
        usrdata_1[i] = 'a';
        usrdata_2[i] = 'b';
    }
    printf("complate usrdata value, switch:%d\n", (*argv[2] -'0'));
    
    filename = argv[1];
    usr_flag = *argv[2] - '0';
    
    fd  = open(filename, O_RDWR);
    if(fd < 0){
        printf("Can't open file %s\r\n", filename);
        return -1;
    }
    if (usr_flag == 1) {
        memcpy(writebuf, usrdata_1, sizeof(usrdata_1));
    } else if (usr_flag == 2) {
        memcpy(writebuf, usrdata_2, sizeof(usrdata_2));
    }
    
    retvalue = write(fd, writebuf, sizeof(writebuf));
    if(retvalue < 0){
        printf("write file %s failed!\r\n", filename);
    }

    memset(readbuf, 0, sizeof(readbuf));
    retvalue = read(fd, readbuf, sizeof(readbuf));
    /*
    if(retvalue < 0){
        printf("read file %s failed!\r\n", filename);
    }else{
        printf("read data:%s\r\n",readbuf);
    }
    */
    for (i = 0; i < 5; i++) 
    {
        sleep(5);
        printf("app is running! \n");
    }
    printf("app run over! \n");
    
    retvalue = read(fd, readbuf, sizeof(readbuf));
    /* 关闭设备 */
    retvalue = close(fd);
    if(retvalue < 0){
        printf("Can't close file %s\r\n", filename);
        return -1;
    }

    return 0;
}



