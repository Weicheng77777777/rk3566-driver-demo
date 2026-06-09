#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
	
	char buf[32] = "123456";
	char ubuf[64] = {0};
	int fd = open("/dev/chardev",O_RDWR);
	if(fd == -1)
	{
		printf("open failed");
		return -1;
	}
    // 1. 准备要发送的数据
    memcpy(ubuf, buf, sizeof(buf));
    printf("准备写入驱动的数据: %s\n", ubuf);

    // 2. 写入驱动
    write(fd, ubuf, sizeof(ubuf));

    // 3. 【重要】清空缓冲区！确保接下来的 read 不是在读旧数据
    memset(ubuf, 0, sizeof(ubuf));
    printf("清空缓冲区后，ubuf的内容为: [%s] (应该是空的)\n", ubuf);

    // 4. 从驱动读回
    read(fd, ubuf, sizeof(ubuf));
    printf("从驱动读回的数据: %s\n", ubuf);

	close(fd);
	return 0;
}


