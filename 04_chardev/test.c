#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main()
{
	char buf[32] = {0};
	int fd = open("/dev/chardev",O_RDWR);
	if(fd == -1)
	{
		printf("open failed\n");
		return -1;
	}
	printf("open success\n");

	read(fd,buf,sizeof(buf));
	printf("read success\n");

	write(fd,buf,sizeof(buf));
	printf("write success\n");

	close(fd);
	printf("close success\n");

	return 0;
}


