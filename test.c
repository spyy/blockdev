#include <unistd.h>
#include <stdio.h>
#include <stropts.h>
#include <fcntl.h>

void main(void)
{

int fd = open("/dev/block_dev", O_RDONLY);
if(fd < 0) printf("error in open\n");

if(ioctl(fd, F_SETFL, 34)) printf("error in ioctl\n");

close(fd);

}
