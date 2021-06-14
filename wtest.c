//#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Make sure uinput is installed and loaded

int main() {
    int fd;

    // May also be /dev/input/uinput
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Failed to open /dev/uinput\n");
        exit(EXIT_FAILURE);
    }

    struct uinput_user_dev uidev;

    memset(&uidev, 0, sizeof(uidev));

    //snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinputsample");
    strncpy(uidev.name, "uinputsample", UINPUT_MAX_NAME_SIZE);
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;


    int err;
    if ((err = ioctl(fd, UI_SET_EVBIT, EV_ABS)) < 0) {
        fprintf(stderr, "Failure setting EV_ABS with err %d\n", err);
        exit(EXIT_FAILURE);
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0) {
        fprintf(stderr, "Failure settings ABS_X\n");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0) {
        fprintf(stderr, "Failure settings ABS_Y\n");
        exit(EXIT_FAILURE);
    }

    // Set min and max on range of absolute inputs
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = 1023;


    /*
    if (write(fd, &uidev, sizeof(uidev)) < 0) {
        fprintf(stderr, "Failed to write device\n");
        exit(EXIT_FAILURE);
    }
    
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        fprintf(stderr, "Failed to create device\n");
        exit(EXIT_FAILURE);
    }
    */

    ioctl(fd, UI_DEV_SETUP, &uidev);
    ioctl(fd, UI_DEV_CREATE);



    struct input_event ev[3];
    struct input_event sev;

    memset(ev, 0, sizeof(ev));

    while (1) {
        ev[0].type = EV_ABS;
        ev[0].code = ABS_X;
        ev[0].value = 1023;
        ev[1].type = EV_ABS;
        ev[1].code = ABS_Y;
        ev[1].value = 767;
        ev[2].type = EV_ABS;
        ev[2].code = ABS_PRESSURE; // Maybe ABS_MT_PRESSURE?
        ev[2].value = 0;

        if (write(fd, &ev, sizeof(ev)) < 0) {
            fprintf(stderr, "Failed to write event\n");
            exit(EXIT_FAILURE);
        }

        sev.type = EV_SYN;
        sev.code = SYN_REPORT;
        sev.value = 0;
        sev.time.tv_sec = 0;
        sev.time.tv_usec = 0;

        write(fd, &sev, sizeof(sev));

        fprintf(stdout, "Event written\n");

        sleep(5);
    }


    if (ioctl(fd, UI_DEV_DESTROY) < 0) {
        fprintf(stderr, "Failed to destroy device\n");
        exit(EXIT_FAILURE);
    }
}
