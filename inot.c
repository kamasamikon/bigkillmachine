#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/unistd.h>
#include <sys/inotify.h>

#define EVENT_MASK (IN_MODIFY)

void watch_file(const char *path)
{
    int fd, wd, i, len, tmp_len;
    char buffer[4096], *offset = NULL;
    struct inotify_event *event;

    fd = inotify_init();
    if (fd < 0) {
        printf("Fail to initialize inotify.\n");
        exit(-1);
    }

    wd_array.name = path;
    wd_array.event = 0;
    wd = inotify_add_watch(fd, wd_array.name, EVENT_MASK);
    if (wd < 0) {
        printf("<%d>: Opt: Configure watcher failed\n", getpid());
        return -1;
    }

    wd_array.wd = wd;

    while (len = read(fd, buffer, sizeof(buffer))) {
        offset = buffer;
        event = (struct inotify_event*)buffer;

        while (((char*)event - buffer) < len) {
            if (event->wd == wd_array.wd) {
                if (!(wd_array.event & event->mask)) {
                    printf("<%d>: Opt: Configure changed\n");
                    /* Modified, reload the configure */
                    printf("T:%08x:%s\n", wd_array.event, wd_array.name);
                }
                break;
            }

            tmp_len = sizeof(struct inotify_event) + event->len;
            event = (struct inotify_event *) (offset + tmp_len);
            offset += tmp_len;
        }
    }
}

int main(int argc, char *argv[])
{
    watch_file(argv[1]);
}
