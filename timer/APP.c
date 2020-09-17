#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, retvalue;
	
	fd = open("/dev/work", O_RDWR);
	if(fd < 0){
		printf("file open failed!\r\n");
		return -1;
	}

	/* 向/dev/led文件写入数据 */
	retvalue = write(fd, "haha", sizeof("haha"));
	if(retvalue < 0){
		printf("LED Control Failed!\r\n");
		close(fd);
		return -1;
	}
    printf("---over---\r\n");
	return 0;
}
