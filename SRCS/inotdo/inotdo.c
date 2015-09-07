/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <poll.h>

static pid_t run_command(char *const argv)
{
	pid_t pid;

	pid = vfork();
	if (0 == pid) {
		/* child process, execute the command */
		printf("================================================================\n");
		system(argv);
		printf("----------------------------------------------------------------\n");
		exit(EXIT_FAILURE);
	}

	return pid;
}

static unsigned int get_mask(const char *maskstr)
{
	char *str = strdup(maskstr), *m;
	unsigned int mask = 0;

	const char *del = ",;:";

	for (m = strtok(str, del); m; m = strtok(NULL, del)) {
		if (!strcasecmp(m, "ACCESS"))
			mask |= IN_ACCESS;
		else if (!strcasecmp(m, "MODIFY"))
			mask |= IN_MODIFY;
		else if (!strcasecmp(m, "ATTRIB"))
			mask |= IN_ATTRIB;
		else if (!strcasecmp(m, "CLOSE_WRITE"))
			mask |= IN_CLOSE_WRITE;
		else if (!strcasecmp(m, "CLOSE_NOWRITE"))
			mask |= IN_CLOSE_NOWRITE;
		else if (!strcasecmp(m, "CLOSE"))
			mask |= IN_CLOSE;
		else if (!strcasecmp(m, "OPEN"))
			mask |= IN_OPEN;
		else if (!strcasecmp(m, "MOVED_FROM"))
			mask |= IN_MOVED_FROM;
		else if (!strcasecmp(m, "MOVED_TO"))
			mask |= IN_MOVED_TO;
		else if (!strcasecmp(m, "MOVE"))
			mask |= IN_MOVE;
		else if (!strcasecmp(m, "CREATE"))
			mask |= IN_CREATE;
		else if (!strcasecmp(m, "DELETE"))
			mask |= IN_DELETE;
		else if (!strcasecmp(m, "DELETE_SELF"))
			mask |= IN_DELETE_SELF;
		else if (!strcasecmp(m, "MOVE_SELF"))
			mask |= IN_MOVE_SELF;
		else if (!strcasecmp(m, "ALL"))
			mask |= IN_ALL_EVENTS;
	}

	free(str);
	return mask;
}

static void dump_event(struct inotify_event *event)
{
	char mask_buff[1024];
	int bytes = 0;

	if (event->mask & IN_ACCESS)
		bytes += sprintf(&mask_buff[bytes], "%s |", "ACCESS");
	if (event->mask & IN_MODIFY)
		bytes += sprintf(&mask_buff[bytes], "%s |", "MODIFY");
	if (event->mask & IN_ATTRIB)
		bytes += sprintf(&mask_buff[bytes], "%s |", "ATTRIB");
	if (event->mask & IN_CLOSE_WRITE)
		bytes += sprintf(&mask_buff[bytes], "%s |", "CLOSE_WRITE");
	if (event->mask & IN_CLOSE_NOWRITE)
		bytes += sprintf(&mask_buff[bytes], "%s |", "CLOSE_NOWRITE");
	if (event->mask & IN_OPEN)
		bytes += sprintf(&mask_buff[bytes], "%s |", "OPEN");
	if (event->mask & IN_MOVED_FROM)
		bytes += sprintf(&mask_buff[bytes], "%s |", "MOVED_FROM");
	if (event->mask & IN_MOVED_TO)
		bytes += sprintf(&mask_buff[bytes], "%s |", "MOVED_TO");
	if (event->mask & IN_CREATE)
		bytes += sprintf(&mask_buff[bytes], "%s |", "CREATE");
	if (event->mask & IN_DELETE)
		bytes += sprintf(&mask_buff[bytes], "%s |", "DELETE");
	if (event->mask & IN_DELETE_SELF)
		bytes += sprintf(&mask_buff[bytes], "%s |", "DELETE_SELF");
	if (event->mask & IN_MOVE_SELF)
		bytes += sprintf(&mask_buff[bytes], "%s |", "MOVE_SELF");

	if (event->mask & IN_UNMOUNT)
		bytes += sprintf(&mask_buff[bytes], "%s |", "UNMOUNT");
	if (event->mask & IN_Q_OVERFLOW)
		bytes += sprintf(&mask_buff[bytes], "%s |", "Q_OVERFLOW");
	if (event->mask & IN_IGNORED)
		bytes += sprintf(&mask_buff[bytes], "%s |", "IGNORED");

	if (event->mask & IN_ONLYDIR)
		bytes += sprintf(&mask_buff[bytes], "%s |", "ONLYDIR");
	if (event->mask & IN_DONT_FOLLOW)
		bytes += sprintf(&mask_buff[bytes], "%s |", "DONT_FOLLOW");
#if 0
	if (event->mask & IN_EXCL_UNLINK)
		bytes += sprintf(&mask_buff[bytes], "%s |", "EXCL_UNLINK");
#endif

	if (event->mask & IN_MASK_ADD)
		bytes += sprintf(&mask_buff[bytes], "%s |", "MASK_ADD");
	if (event->mask & IN_ISDIR)
		bytes += sprintf(&mask_buff[bytes], "%s |", "ISDIR");
	if (event->mask & IN_ONESHOT)
		bytes += sprintf(&mask_buff[bytes], "%s |", "ONESHOT");

	printf("> name : %s\n", (event->len > 0) ? event->name : "(null)");

	if (bytes) {
		mask_buff[bytes - 2] = '\0';
		printf("> mask : %s\n\n", mask_buff);
	} else
		printf("> mask : %s\n\n", "(null)");
}

static void dump_events(struct inotify_event *events, int len)
{
	unsigned char *ptr = (unsigned char*)events;
	int offset;

	struct inotify_event *event;

	offset = 0;
	while (offset < len) {
		event = (struct inotify_event*)(void*)(&ptr[offset]);
		dump_event(event);

		offset += sizeof(struct inotify_event) + event->len;
	}
}

int main(int argc, char *argv[])
{
	int fd, wd;
	struct inotify_event events[1024];
	unsigned int mask;
	int bytes;

	int verbose;

	if (argc < 4) {
		printf("usage: inotdo file mask command ...\n");
		printf("mask:\n");
		printf("        access : File was accessed.\n");
		printf("        modify : File was modified.\n");
		printf("        attrib : Metadata changed.\n");
		printf("   close_write : Writtable file was closed.\n");
		printf(" close_nowrite : Unwrittable file closed.\n");
		printf("         close : (close_write | close_nowrite)\n");
		printf("          open : File was opened.\n");
		printf("    moved_from : File was moved from X.\n");
		printf("      moved_to : File was moved to Y.\n");
		printf("          move : (moved_from | moved_to)\n");
		printf("        create : Subfile was created.\n");
		printf("        delete : Subfile was deleted.\n");
		printf("   delete_self : Self was deleted.\n");
		printf("     move_self : Self was moved.\n");
		printf("           all : Ored all above.\n");
		printf("\n");
		printf("env[WBG_DEBUG] : Show verbose.\n");
		printf("\n");
		printf("e.g.: inotdo /tmp/abc access,move 'echo \"xxx\"; ls'\n");
		return -1;
	}

	fd = inotify_init();
	if (fd < 0) {
		fprintf(stderr, "inotify_init NG.\n");
		return -1;
	}

	mask = get_mask(argv[2]);
	wd = inotify_add_watch(fd, argv[1], mask);
	if (wd < 0) {
		fprintf(stderr, "inotify_add_watch NG\n");
		return -1;
	}

	if (getenv("WBG_DEBUG"))
		verbose = 1;
	else
		verbose = 0;

	while (1) {
		struct pollfd pfd = { fd, POLLIN, 0 };
		int ret = poll(&pfd, 1, 50);
		if (ret < 0) {
			fprintf(stderr, "poll failed: %s\n", strerror(errno));
		} else if (ret == 0) {
			// Timeout with no events, move on.
		} else {
			bytes = read(fd, (void*)&events, sizeof(events));
			if (-1 == bytes) {
				fprintf(stderr, "read NG: %s\n", strerror(errno));
				goto clean_quit;
			}

			if (verbose)
				dump_events(events, bytes);
			run_command(argv[3]);
		}
	}

clean_quit:
	inotify_rm_watch(fd, wd);
	close(fd);

	return 0;
}

