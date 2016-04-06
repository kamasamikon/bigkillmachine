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
	int wd;
	int msc_idx;
	int cmd_idx;
	char *file;
};

static int __argc;
static char **__argv;

static watchinf_s *__wchinf = NULL;
static int __wchinf_cnt = 0;

static int __debug_mode = 0;

static void set_subenv(struct inotify_event *event);

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

static pid_t run_command(char *const argv, struct inotify_event *event)
{
	pid_t pid;
	int ret;

	pid = fork();
	if (0 == pid) {
		set_subenv(event);

		/* child process, execute the command */
#if 0
		execvp(argv[0], argv);
		_exit(EXIT_FAILURE);
#else
		printf("================================================================\n");
		ret = system(argv);
		printf("----------------------------------------------------------------\n");
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

static void dump_event(struct inotify_event *event)
{
	char mask[1024];
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

	watchinf_s *wi = watchinf_find(event->wd);
	if (wi)
		logmsg("> file : %s\n", wi->file);


	if (bytes) {
		mask[bytes - 2] = '\0';
		logmsg("> mask :%s\n\n", mask);
	} else {
		logmsg("> mask :%s\n\n", " (null)");
	}
}

static void set_subenv(struct inotify_event *event)
{
	char mask[1024];
	int bytes = 0;

	char envname[256], envmask[256];

	sprintf(envname, "WBG_NAME");
	sprintf(envmask, "WBG_MASK");

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

	watchinf_s *wi = watchinf_find(event->wd);
	if (wi)
		setenv(envname, wi->file, 1);

	if (bytes) {
		mask[bytes - 2] = '\0';
		setenv(envmask, &mask[1], 1);
	} else {
		setenv(envmask, "(null)", 1);
	}
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

	__argc = argc;
	__argv = argv;

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
		printf("e.g.: inotdo -m1 access,move -ca 'echo \"xxx\"; ls' -r,1,a /tmp/abc ~/haha.c ... \n");
		return -1;
	}

	__msc_name = (char**)calloc(argc, sizeof(char*));
	__msc_content = (char**)calloc(argc, sizeof(char*));
	__cmd_name = (char**)calloc(argc, sizeof(char*));
	__cmd_content = (char**)calloc(argc, sizeof(char*));

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];

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
			printf("Bad mask '%s' for [%s].\n", cur_mask, arg);
			continue;
		}

		for (k = 0; k < __cmd_idx; k++) {
			if (!strcmp(cur_comm, __cmd_name[k]))
				break;
		}
		if (k == __cmd_idx) {
			printf("Bad command '%s' for [%s].\n", cur_comm, arg);
			continue;
		}

		mask = get_mask(__msc_content[j]);
		wd = inotify_add_watch(fd, file, mask);
		if (wd < 0) {
			fprintf(stderr, "inotify_add_watch NG: FILE:'%s' ERR:%s\n",
					argv[i], strerror(errno));
			continue;
		}

		wi = &__wchinf[__wchinf_cnt++];
		wi->wd = wd;
		wi->msc_idx = j;
		wi->cmd_idx = k;
		wi->file = file;
	}


	size_t evbuflen = argc * 2 * sizeof(struct inotify_event);
	unsigned char *evbuf = (unsigned char*)malloc(evbuflen);

	if (getenv("WBG_DEBUG")) {
		printf("WBG: WBG_DEBUG set.\n");
		__debug_mode = 1;
	}

	while (1) {
		struct pollfd pfd = { fd, POLLIN, 0 };
		int ret = poll(&pfd, 1, poll_timeout);
		if (ret < 0) {
			fprintf(stderr, "poll failed: %s\n", strerror(errno));
			continue;
		}

		if (ret > 0) {
			bytes = read(fd, (void*)evbuf, evbuflen);
			if (-1 == bytes) {
				fprintf(stderr, "read NG: %s\n", strerror(errno));
				goto clean_quit;
			}

			int i, ofs = 0;
			struct inotify_event *ev;

			while (ofs < bytes) {
				ev = (struct inotify_event*)(void*)(&evbuf[ofs]);
				if (__debug_mode)
					dump_event(ev);

				for (i = 0; i < __wchinf_cnt; i++) {
					wi = &__wchinf[i];
					if (wi->wd != ev->wd)
						continue;

					command = __cmd_content[wi->cmd_idx];
					logmsg("WD:%d, CMD: [%s]\n", wi->wd, command);
					if (command && command[0])
						run_command(command, ev);
					break;
				}

				ofs += sizeof(struct inotify_event) + ev->len;
			}

		}
	}

clean_quit:
	if (__wchinf) {
		for (i = 0; i < argc - 3; i++) {
			wi = &__wchinf[i];
			if (wi->wd < 0)
				break;
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

