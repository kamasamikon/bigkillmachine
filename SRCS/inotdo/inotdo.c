/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <poll.h>
#include <stdarg.h>

typedef struct _watchinf_s watchinf_s;
struct _watchinf_s {
	unsigned int mask;
	int wd;
	int msc_idx;
	int cmd_idx;
	char *file;

	/* Event occured for this */
	int dirty;
};

static int __argc;
static char **__argv;

static watchinf_s *__wchinf = NULL;
static int __wchinf_cnt = 0;

static int __debug_mode = 0;
static int __silent_call = 0;

static void set_subenv(struct inotify_event *ev);

/* Mask array and Command arrary */
int __msc_cnt = 0;
char **__msc_name, **__msc_content;

int __cmd_idx = 0;
char **__cmd_name, **__cmd_content;

static void logmsg(const char *fmt, ...)
{
	va_list ap;

	if (__debug_mode) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}

static watchinf_s *watchinf_find(int wd)
{
	int i;
	watchinf_s *wi;

	for (i = 0; i < __wchinf_cnt; i++) {
		wi = &__wchinf[i];
		if (wi->wd < 0)
			return NULL;
		if (wi->wd == wd)
			return wi;
	}
	return NULL;
}

static unsigned int uptime(void)
{
	FILE *fp = fopen("/proc/uptime", "r");
	unsigned int retval = 0;
	char tmp[64] = { 0 };
	char *res = NULL;

	if (fp) {
		res = fgets(tmp, sizeof(tmp), fp);
		retval = (int)(atof(tmp)) * 1000;
		fclose(fp);
	}

	return retval;
}

static pid_t run_command(char *const argv, struct inotify_event *ev)
{
	pid_t pid;
	int ret;

	pid = fork();
	if (0 == pid) {
		set_subenv(ev);

		/* child process, execute the command */
#if 0
		execvp(argv[0], argv);
		_exit(EXIT_FAILURE);
#else
		if (!__silent_call)
			printf(">>> WBG: '%s' start.\n", argv);
		ret = system(argv);
		if (!__silent_call)
			printf("<<< WBG: '%s' end.\n", argv);
		exit(ret);
#endif
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

static void event_parse(struct inotify_event *ev,
		char file[1024], char name[1024], char mask[1024])
{
	char buf[1024];
	int bytes = 0;

	file[0] = '\0';
	name[0] = '\0';
	mask[0] = '\0';

	if (ev->mask & IN_ACCESS)
		bytes += sprintf(&buf[bytes], " %s |", "ACCESS");
	if (ev->mask & IN_MODIFY)
		bytes += sprintf(&buf[bytes], " %s |", "MODIFY");
	if (ev->mask & IN_ATTRIB)
		bytes += sprintf(&buf[bytes], " %s |", "ATTRIB");
	if (ev->mask & IN_CLOSE_WRITE)
		bytes += sprintf(&buf[bytes], " %s |", "CLOSE_WRITE");
	if (ev->mask & IN_CLOSE_NOWRITE)
		bytes += sprintf(&buf[bytes], " %s |", "CLOSE_NOWRITE");
	if (ev->mask & IN_OPEN)
		bytes += sprintf(&buf[bytes], " %s |", "OPEN");
	if (ev->mask & IN_MOVED_FROM)
		bytes += sprintf(&buf[bytes], " %s |", "MOVED_FROM");
	if (ev->mask & IN_MOVED_TO)
		bytes += sprintf(&buf[bytes], " %s |", "MOVED_TO");
	if (ev->mask & IN_CREATE)
		bytes += sprintf(&buf[bytes], " %s |", "CREATE");
	if (ev->mask & IN_DELETE)
		bytes += sprintf(&buf[bytes], " %s |", "DELETE");
	if (ev->mask & IN_DELETE_SELF)
		bytes += sprintf(&buf[bytes], " %s |", "DELETE_SELF");
	if (ev->mask & IN_MOVE_SELF)
		bytes += sprintf(&buf[bytes], " %s |", "MOVE_SELF");

	if (ev->mask & IN_UNMOUNT)
		bytes += sprintf(&buf[bytes], " %s |", "UNMOUNT");
	if (ev->mask & IN_Q_OVERFLOW)
		bytes += sprintf(&buf[bytes], " %s |", "Q_OVERFLOW");
	if (ev->mask & IN_IGNORED)
		bytes += sprintf(&buf[bytes], " %s |", "IGNORED");

	if (ev->mask & IN_ONLYDIR)
		bytes += sprintf(&buf[bytes], " %s |", "ONLYDIR");
	if (ev->mask & IN_DONT_FOLLOW)
		bytes += sprintf(&buf[bytes], " %s |", "DONT_FOLLOW");
#if 0
	if (ev->mask & IN_EXCL_UNLINK)
		bytes += sprintf(&buf[bytes], "%s |", "EXCL_UNLINK");
#endif

	if (ev->mask & IN_MASK_ADD)
		bytes += sprintf(&buf[bytes], " %s |", "MASK_ADD");
	if (ev->mask & IN_ISDIR)
		bytes += sprintf(&buf[bytes], " %s |", "ISDIR");
	if (ev->mask & IN_ONESHOT)
		bytes += sprintf(&buf[bytes], " %s |", "ONESHOT");

	watchinf_s *wi = watchinf_find(ev->wd);
	if (wi)
		strcpy(file, wi->file);

	if (ev->len)
		strcpy(name, ev->name);

	if (bytes) {
		buf[bytes - 2] = '\0';
		strcpy(mask, &buf[1]);
	}
}

static void dump_event(struct inotify_event *ev)
{
	char file[1024], name[1024], mask[1024];

	event_parse(ev, file, name, mask);

	if (file[0])
		logmsg("> file: %s\n", file);

	if (name[0])
		logmsg("> name: %s\n", name);

	logmsg("> mask: %s\n", mask[0] ? mask : " (null)");
}


static void set_subenv(struct inotify_event *ev)
{
	char file[1024], name[1024], mask[1024];

	event_parse(ev, file, name, mask);

	if (file[0])
		setenv("WBG_FILE", file, 1);
	else
		unsetenv("WBG_FILE");

	if (name[0])
		setenv("WBG_NAME", name, 1);
	else
		unsetenv("WBG_NAME");

	if (mask[0])
		setenv("WBG_MASK", mask, 1);
	else
		unsetenv("WBG_MASK");
}


static int rule_split(const char *rule, char mask[256], char comm[256])
{
	int i;
	const char *p = rule;

	if ((*p++ != '-'))
		return -1;

	if ((*p++ != 'r'))
		return -1;

	if (!*p) {
		mask[0] = '\0';
		comm[0] = '\0';
		return 0;
	}
	if (*p != ',')
		return -1;

	for (i = 0, p++; *p && *p != ','; p++) {
		mask[i++] = *p;
	}
	mask[i] = '\0';

	if (!*p) {
		comm[0] = '\0';
		return 0;
	}

	for (i = 0, p++; *p && *p != ','; p++) {
		comm[i++] = *p;
	}
	comm[i] = '\0';

	return 0;
}

int main(int argc, char *argv[])
{
	watchinf_s *wi = NULL;
	char cur_mask[256], cur_comm[256];

	int fd, wd, i, j, k, bytes;
	unsigned int mask;
	char *command;

	int poll_timeout = 1000;

	int reload_mode = 0;

	__argc = argc;
	__argv = argv;

	if (argc < 4) {
		printf("usage: inotdo [-reload] [-token token-string] [-s] [-v | -nov] -m<mask> mask [-c<cmd> command] -r,<mask>,<cmd> files ...\n");
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
		printf("        -token : A string.\n");
		printf("       -reload : Re-watch the file after hit with same mask.\n");
		printf("            -s : Slient mode: No log message when call the commands.\n");
		printf("     -v / -nov : Verbose mode on and off.\n");
		printf("\n");
		printf("env[WBG_DEBUG] : Show verbose.\n");
		printf("\n");
		printf("e.g.: inotdo -m1 access,move -ca 'echo \"xxx\"; ls' -r,1,a /tmp/abc ~/haha.c ... \n");
		return -1;
	}

	if (getenv("WBG_DEBUG")) {
		printf("WBG: WBG_DEBUG set.\n");
		__debug_mode = 1;
	}

	__msc_name = (char**)calloc(argc, sizeof(char*));
	__msc_content = (char**)calloc(argc, sizeof(char*));
	__cmd_name = (char**)calloc(argc, sizeof(char*));
	__cmd_content = (char**)calloc(argc, sizeof(char*));

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];

		if (!strcmp(arg, "-token")) {
			argv[i] = NULL;
			argv[i + 1] = NULL;

			i++;
		}

		if (!strcmp(arg, "-reload")) {
			argv[i] = NULL;
			reload_mode = 1;
		}

		if (!strcmp(arg, "-s")) {
			argv[i] = NULL;
			__silent_call = 1;
		}
		if (!strcmp(arg, "-v")) {
			argv[i] = NULL;
			__debug_mode = 1;
		}
		if (!strcmp(arg, "-nov")) {
			argv[i] = NULL;
			__debug_mode = 0;
		}

		if (arg[0] == '-' && arg[1] == 'm') {
			__msc_name[__msc_cnt] = &arg[2];
			__msc_content[__msc_cnt] = argv[i + 1];
			__msc_cnt++;

			argv[i] = NULL;
			argv[i + 1] = NULL;

			i++;
		}

		if (arg[0] == '-' && arg[1] == 'c') {
			__cmd_name[__cmd_idx] = &arg[2];
			__cmd_content[__cmd_idx] = argv[i + 1];
			__cmd_idx++;

			argv[i] = NULL;
			argv[i + 1] = NULL;

			i++;
		}
	}

	fd = inotify_init();
	if (fd < 0) {
		fprintf(stderr, "inotify_init NG.\n");
		return -1;
	}

	cur_mask[0] = '\0';
	cur_comm[0] = '\0';
	__wchinf = (watchinf_s*)calloc(argc, sizeof(watchinf_s));
	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		char *file = argv[i + 1];

		if (!arg) {
			/* reset current mask and command */
			cur_mask[0] = '\0';
			cur_comm[0] = '\0';

			/* skip stripped -m and -c */
			continue;
		}

		if (rule_split(arg, cur_mask, cur_comm))
			file = arg;
		else
			i++;

		if (!file)
			break;

		logmsg("cur_mask:%c:%s\n", cur_mask[0], cur_mask);
		logmsg("cur_comm:%c:%s\n", cur_comm[0], cur_comm);

		for (j = 0; j < __msc_cnt; j++) {
			if (!strcmp(cur_mask, __msc_name[j]))
				break;
		}
		if (j == __msc_cnt) {
			printf("Bad mask '%s' for [%s].\n", cur_mask, file);
			continue;
		}

		for (k = 0; k < __cmd_idx; k++) {
			if (!strcmp(cur_comm, __cmd_name[k]))
				break;
		}
		if (k == __cmd_idx) {
			printf("No command '%s' for [%s].\n", cur_comm, file);
			k = -1;
		}

		mask = get_mask(__msc_content[j]);
		wd = inotify_add_watch(fd, file, mask);
		if (wd < 0) {
			fprintf(stderr, "inotify_add_watch NG: FILE:'%s' ERR:%s\n",
					argv[i], strerror(errno));
			continue;
		}

		wi = &__wchinf[__wchinf_cnt++];
		wi->mask = mask;
		wi->wd = wd;
		wi->msc_idx = j;
		wi->cmd_idx = k;
		wi->file = file;
		wi->dirty = 0;
	}


	size_t evbuflen = argc * 2 * sizeof(struct inotify_event);
	unsigned char *evbuf = (unsigned char*)malloc(evbuflen);

	while (1) {
		struct pollfd pfd = { fd, POLLIN, 0 };
		int ret = poll(&pfd, 1, poll_timeout);
		if (ret < 0) {
			/* Error */
			fprintf(stderr, "poll failed: %s\n", strerror(errno));
			continue;
		}
		if (ret == 0) {
			/* Timeout */
			continue;
		}

		bytes = read(fd, (void*)evbuf, evbuflen);
		if (-1 == bytes) {
			fprintf(stderr, "read NG: %s\n", strerror(errno));
			goto clean_quit;
		}

		int i, ofs = 0;
		struct inotify_event *ev;

		while (ofs < bytes) {
			ev = (struct inotify_event*)(void*)(&evbuf[ofs]);

			for (i = 0; i < __wchinf_cnt; i++) {
				wi = &__wchinf[i];
				if (wi->wd != ev->wd)
					continue;

				wi->dirty = 1;

				/* IN_IGNORED etc should be skipped */
				if (!(wi->mask & ev->mask))
					continue;

				if (__debug_mode) {
					logmsg("\nWD:%d, CMD: [%s]\n", wi->wd, command);
					dump_event(ev);
				}

				if (wi->cmd_idx >= 0)
					command = __cmd_content[wi->cmd_idx];
				else
					command = NULL;

				if (command && command[0])
					run_command(command, ev);
			}

			ofs += sizeof(struct inotify_event) + ev->len;
		}

		/*
		 * XXX: Delete and readd, it will
		 * caused many INGORED event
		 */
		if (reload_mode) {
			for (i = 0; i < argc - 3; i++) {
				wi = &__wchinf[i];
				if (!wi->dirty)
					continue;

				wi->dirty = 0;
				inotify_rm_watch(fd, wi->wd);
				wi->wd = inotify_add_watch(fd, wi->file, wi->mask);
			}
		}
	}

clean_quit:
	if (__wchinf) {
		for (i = 0; i < argc - 3; i++) {
			wi = &__wchinf[i];
			inotify_rm_watch(fd, wd);
		}
		free(__wchinf);
	}

	if (__msc_name)
		free(__msc_name);
	if (__msc_content)
		free(__msc_content);
	if (__cmd_name)
		free(__cmd_name);
	if (__cmd_content)
		free(__cmd_content);

	close(fd);
	free(evbuf);

	return 0;
}

