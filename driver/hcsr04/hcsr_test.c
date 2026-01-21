// hcsr_test.c - simple test: read distance (cm) from /dev/hcsr04
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

int main()
{
    int fd = open("/dev/hcsr04", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/hcsr04");
        return 1;
    }

    while (1) {
        int dist;
        ssize_t r = read(fd, &dist, sizeof(dist));
        if (r == sizeof(dist)) {
            printf("distance = %d cm\n", dist);
        } else {
            if (r < 0) {
                if (errno == ETIME)
                    printf("measurement timeout\n");
                else
                    printf("read error: %s\n", strerror(errno));
            } else {
                printf("short read: %zd\n", r);
            }
        }
        usleep(300000); // 300ms between measurements
    }

    close(fd);
    return 0;
}
