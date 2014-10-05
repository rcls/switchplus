#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>

#include "util.h"

int main(int argc, char **argv)
{
    if (argc != 2)
        errx(1, "Usage: usbreset device-filename");

    int fd = checkz(open(argv[1], O_WRONLY), "open");

    checkz(ioctl(fd, USBDEVFS_RESET, 0), "ioctl");

    close(fd);
    return 0;
}
