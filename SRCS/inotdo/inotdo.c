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
	int ret;

	pid = vfork();
	if (0 == pid) {
		/* child process, execute the command */
		printf("================================================================\n");
		ret = system(argv);
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
	char mask[1024], *name;
	int bytes = 0;

	if (event->mask & IN_ACCESS)
		bytes += sprintf(&mask[bytes], " %s |", "ACCESS");
	if (event->mask & IN_MODIFY)
		bytes += sprintf(&mask[bytes], " %s |", "MODIFY");
	if (event->mask & IN_ATTRIB)
		bytes += sprintf(&mask[bytes], " %s |", "ATTRIB");
	if (event->mask & IN_CLOSE_WRITE)
		bytes += sprintf(&mask[bytes], " %s |", "CLOSE_WRITE");
	if (event->mask & IN_CLOSE_NOWRITE)
		bytes += sprintf(&mask[bytes], " %s |", "CLOSE_NOWRITE");
	if (event->mask & IN_OPEN)
		bytes += sprintf(&mask[bytes], " %s |", "OPEN");
	if (event->mask & IN_MOVED_FROM)
		bytes += sprintf(&mask[bytes], " %s |", "MOVED_FROM");
	if (event->mask & IN_MOVED_TO)
		bytes += sprintf(&mask[bytes], " %s |", "MOVED_TO");
	if (event->mask & IN_CREATE)
		bytes += sprintf(&mask[bytes], " %s |", "CREATE");
	if (event->mask & IN_DELETE)
		bytes += sprintf(&mask[bytes], " %s |", "DELETE");
	if (event->mask & IN_DELETE_SELF)
		bytes += sprintf(&mask[bytes], " %s |", "DELETE_SELF");
	if (event->mask & IN_MOVE_SELF)
		bytes += sprintf(&mask[bytes], " %s |", "MOVE_SELF");

	if (event->mask & IN_UNMOUNT)
		bytes += sprintf(&mask[bytes], " %s |", "UNMOUNT");
	if (event->mask & IN_Q_OVERFLOW)
		bytes += sprintf(&mask[bytes], " %s |", "Q_OVERFLOW");
	if (event->mask & IN_IGNORED)
		bytes += sprintf(&mask[bytes], " %s |", "IGNORED");

	if (event->mask & IN_ONLYDIR)
		bytes += sprintf(&mask[bytes], " %s |", "ONLYDIR");
	if (event->mask & IN_DONT_FOLLOW)
		bytes += sprintf(&mask[bytes], " %s |", "DONT_FOLLOW");
#if 0
	if (event->mask & IN_EXCL_UNLINK)
		bytes += sprintf(&mask[bytes], "%s |", "EXCL_UNLINK");
#endif

	if (event->mask & IN_MASK_ADD)
		bytes += sprintf(&mask[bytes], " %s |", "MASK_ADD");
	if (event->mask & IN_ISDIR)
		bytes += sprintf(&mask[bytes], " %s |", "ISDIR");
	if (event->mask & IN_ONESHOT)
		bytes += sprintf(&mask[bytes], " %s |", "ONESHOT");

	name = (event->len > 0) ? event->name : "(null)";
	printf("> name : %s\n", name);

	if (bytes) {
		mask[bytes - 2] = '\0';
		printf("> mask :%s\n\n", mask);
	} else {
		printf("> mask :%s\n\n", " (null)");
	}
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

static void set_subenv(struct inotify_event *event, int index)
{
	char mask[1024], *name;
	int bytes = 0;

	char envname[256], envmask[256];

	sprintf(envname, "WBG_NAME_%d", index);
	sprintf(envmask, "WBG_MASK_%d", index);

	if (event->mask & IN_ACCESS)
		bytes += sprintf(&mask[bytes], " %s |", "ACCESS");
	if (event->mask & IN_MODIFY)
		bytes += sprintf(&mask[bytes], " %s |", "MODIFY");
	if (event->mask & IN_ATTRIB)
		bytes += sprintf(&mask[bytes], " %s |", "ATTRIB");
	if (event->mask & IN_CLOSE_WRITE)
		bytes += sprintf(&mask[bytes], " %s |", "CLOSE_WRITE");
	if (event->mask & IN_CLOSE_NOWRITE)
		bytes += sprintf(&mask[bytes], " %s |", "CLOSE_NOWRITE");
	if (event->mask & IN_OPEN)
		bytes += sprintf(&mask[bytes], " %s |", "OPEN");
	if (event->mask & IN_MOVED_FROM)
		bytes += sprintf(&mask[bytes], " %s |", "MOVED_FROM");
	if (event->mask & IN_MOVED_TO)
		bytes += sprintf(&mask[bytes], " %s |", "MOVED_TO");
	if (event->mask & IN_CREATE)
		bytes += sprintf(&mask[bytes], " %s |", "CREATE");
	if (event->mask & IN_DELETE)
		bytes += sprintf(&mask[bytes], " %s |", "DELETE");
	if (event->mask & IN_DELETE_SELF)
		bytes += sprintf(&mask[bytes], " %s |", "DELETE_SELF");
	if (event->mask & IN_MOVE_SELF)
		bytes += sprintf(&mask[bytes], " %s |", "MOVE_SELF");

	if (event->mask & IN_UNMOUNT)
		bytes += sprintf(&mask[bytes], " %s |", "UNMOUNT");
	if (event->mask & IN_Q_OVERFLOW)
		bytes += sprintf(&mask[bytes], " %s |", "Q_OVERFLOW");
	if (event->mask & IN_IGNORED)
		bytes += sprintf(&mask[bytes], " %s |", "IGNORED");

	if (event->mask & IN_ONLYDIR)
		bytes += sprintf(&mask[bytes], " %s |", "ONLYDIR");
	if (event->mask & IN_DONT_FOLLOW)
		bytes += sprintf(&mask[bytes], " %s |", "DONT_FOLLOW");
#if 0
	if (event->mask & IN_EXCL_UNLINK)
		bytes += sprintf(&mask[bytes], "%s |", "EXCL_UNLINK");
#endif

	if (event->mask & IN_MASK_ADD)
		bytes += sprintf(&mask[bytes], " %s |", "MASK_ADD");
	if (event->mask & IN_ISDIR)
		bytes += sprintf(&mask[bytes], " %s |", "ISDIR");
	if (event->mask & IN_ONESHOT)
		bytes += sprintf(&mask[bytes], " %s |", "ONESHOT");

	name = (event->len > 0) ? event->name : "(null)";
	setenv(envname, name, 1);

	if (bytes) {
		mask[bytes - 2] = '\0';
		setenv(envmask, &mask[1], 1);
	} else {
		setenv(envmask, "(null)", 1);
	}
}

static void unset_old_env()
{
	char *wbg_events;
	int i, count;
	char env[255];

	wbg_events = getenv("WBG_EVENTS");
	if (!wbg_events)
		return;

	count = atoi(wbg_events);
	for (i = 0; i < count; i++) {
		snprintf(env, sizeof(env), "WBG_NAME_%d", i);
		unsetenv(env);
		snprintf(env, sizeof(env), "WBG_MASK_%d", i);
		unsetenv(env);
	}
	unsetenv("WBG_EVENTS");
}

static void set_subenvs(struct inotify_event *events, int len)
{
	unsigned char *ptr = (unsigned char*)events;
	int offset, index;
	char count[24];

	struct inotify_event *event;

	unset_old_env();

	offset = 0;
	index = 0;
	while (offset < len) {
		event = (struct inotify_event*)(void*)(&ptr[offset]);
		set_subenv(event, index);

		offset += sizeof(struct inotify_event) + event->len;
		index++;
	}

	sprintf(count, "%d", index);
	setenv("WBG_EVENTS", count, 1);
}

int main(int argc, char *argv[])
{
	int fd, wd;
	struct inotify_event events[1024];
	unsigned int mask;
	int bytes;

	char *commandline;

	int verbose;

	if (argc < 3) {
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

	if (argc > 3)
		commandline = argv[3];
	else
		commandline = NULL;

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

			if (commandline) {
				set_subenvs(events, bytes);
				run_command(commandline);
			}
		}
	}

clean_quit:
	inotify_rm_watch(fd, wd);
	close(fd);

	return 0;
}

