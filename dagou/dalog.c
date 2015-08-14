/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#include <helper.h>
#include <dalog.h>
#include <nbuf.h>
#include <narg.h>

extern ssize_t getline(char **lineptr, size_t *n, FILE *stream);

static inline void *dalog_cc(void);
static void dalog_init_default(void);

static char *get_execpath(int *size);
static char *get_basename(char *name);
static char *get_progname();

static unsigned int __dft_mask = DALOG_ALL;

/*-----------------------------------------------------------------------
 * Local definition:
 */

/* STRing ARRay to save programe name, module name, file name etc */
typedef struct _strarr_s strarr_s;
struct _strarr_s {
	int size;
	int cnt;
	char **arr;
};

typedef struct _rule_s rule_s;
struct _rule_s {
	/* XXX: Don't filter ThreadID, it need the getflg called every log */

	/* 0 is know care */
	/* -1 is all */

	/* XXX: the prog modu file func is return from strarr_add */
	char *prog;	/* Program command line */
	char *modu;	/* Module name */
	char *file;	/* File name */
	char *func;	/* Function name */

	int line;	/* Line number */

	int pid;	/* Process ID */

	/* Which flag to be set or clear */
	unsigned int set, clr;
};

typedef struct _rulearr_s rulearr_s;
struct _rulearr_s {
	unsigned int size;
	unsigned int cnt;
	rule_s *arr;
};

/* How many logger slot */
#define MAX_NLOGGER 8
#define MAX_RLOGGER 8

/* Control Center for dalog */
typedef struct _dalogcc_s dalogcc_s;
struct _dalogcc_s {
	/** version is a ref count user change dalog arg */
	int touches;

	pid_t pid;

	pthread_mutex_t mutex;

	strarr_s arr_file_name;
	strarr_s arr_modu_name;
	strarr_s arr_prog_name;
	strarr_s arr_func_name;

	rulearr_s arr_rule;

	unsigned char nlogger_cnt, rlogger_cnt;
	DAL_NLOGGER nloggers[MAX_NLOGGER];
	DAL_RLOGGER rloggers[MAX_RLOGGER];
};

static dalogcc_s *__g_dalogcc = NULL;

/*-----------------------------------------------------------------------
 * Control Center
 */
static inline void *dalog_cc(void)
{
	if (dalog_unlikely(!__g_dalogcc))
		dalog_init_default();

	return (void*)__g_dalogcc;
}

/*-----------------------------------------------------------------------
 * dalog-logger
 */
int dalog_add_logger(DAL_NLOGGER logger)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i;

	for (i = 0; i < MAX_NLOGGER; i++)
		if (cc->nloggers[i] == logger)
			return 0;

	if (cc->nlogger_cnt >= MAX_NLOGGER) {
		fprintf(stderr, "dalog_add_logger: Only up to %d logger supported.\n", MAX_NLOGGER);
		return -1;
	}

	cc->nloggers[cc->nlogger_cnt++] = logger;
	return 0;
}

int dalog_del_logger(DAL_NLOGGER logger)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i] == logger) {
			cc->nlogger_cnt--;
			cc->nloggers[i] = cc->nloggers[cc->nlogger_cnt];
			cc->nloggers[cc->nlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}

int dalog_add_rlogger(DAL_RLOGGER logger)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i;

	for (i = 0; i < MAX_RLOGGER; i++)
		if (cc->rloggers[i] == logger)
			return 0;

	if (cc->rlogger_cnt >= MAX_RLOGGER) {
		fprintf(stderr, "dalog_add_logger: Only up to %d logger supported.\n", MAX_RLOGGER);
		return -1;
	}

	cc->rloggers[cc->rlogger_cnt++] = logger;
	return 0;
}

int dalog_del_rlogger(DAL_RLOGGER logger)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i] == logger) {
			cc->rlogger_cnt--;
			cc->rloggers[i] = cc->rloggers[cc->rlogger_cnt];
			cc->rloggers[cc->rlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}

/*-----------------------------------------------------------------------
 * implementation
 */
static void dalog_parse_mask(char *mask, unsigned int *set, unsigned int *clr);

static uint64_t os_uptime()
{
	struct sysinfo info;
	struct timeval tv;
	uint64_t ms;

	sysinfo(&info);
	gettimeofday(&tv,NULL);

	ms = (uint64_t)info.uptime;
	ms *= 1000;
	ms += tv.tv_usec / 1000;

	return ms;
}

/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *dalog_attach(void *logcc)
{
	__g_dalogcc = (dalogcc_s*)logcc;
	return (void*)__g_dalogcc;
}

static char *get_execpath(int *size)
{
	nbuf_s nb;
	FILE *fp;
	char buff[256];
	int err = 0;
	char *path = NULL;

	sprintf(buff, "/proc/%d/cmdline", getpid());
	fp = fopen(buff, "rt");
	if (fp) {
		nbuf_init(&nb, 0);

		while (nbuf_fread(&nb, 1024, fp, &err) > 0 && !err)
			;
		fclose(fp);

		if (size)
			*size = nb.len;
		nb.buf[nb.len] = '\0';

		path = nbuf_detach(&nb, NULL);
	}

	nbuf_release(&nb);
	return path;
}

static void dalog_init_default()
{
	int argc, cl_size = 0;
	char **argv, *cl_buf;

	if (__g_dalogcc)
		return;

	argc = 0;
	argv = NULL;

	cl_buf = get_execpath(&cl_size);
	narg_build_nul(cl_buf, cl_size, &argc, &argv);
	nmem_free(cl_buf);

	dalog_init(argc, argv);
	narg_free(argc, argv);
}

void dalog_touch(void)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();

	cc->touches++;
}
inline int dalog_touches(void)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();

	return cc->touches;
}

static char *get_basename(char *name)
{
	char *dupname, *bn;

	if (!name)
		return strdup("");

	dupname = strdup(name);
	bn = basename(dupname);

	bn = bn ? strdup(bn) : strdup("");

	nmem_free_s(dupname);

	return bn;
}

static char *get_progname()
{
	static char prog_name_buff[64];
	static char *prog_name = NULL;

	char *execpath, *execname;

	if (dalog_likely(prog_name))
		return prog_name;

	execpath = get_execpath(NULL);
	if (execpath) {
		execname = get_basename(execpath);
		strncpy(prog_name_buff, execname, sizeof(prog_name_buff));
		nmem_free(execpath);
		nmem_free(execname);
	} else
		snprintf(prog_name_buff, sizeof(prog_name_buff) - 1,
				"PROG-%d", (int)getpid());

	prog_name = prog_name_buff;
	return prog_name;
}

static void load_cfg_file(char *path)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes;

	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp) {
		fprintf(stderr, "load_cfg_file: fopen '%s' failed, e:%d\n", path, errno);
		return;
	}

	while ((bytes = getline(&line, &len, fp)) != -1)
		dalog_rule_add(line);

	fclose(fp);
	nmem_free_s(line);
}

static void apply_rtcfg(char *path)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes;

	static int line_applied = 0;
	int line_cnt = 0;
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while ((bytes = getline(&line, &len, fp)) != -1) {
		if (line_cnt++ < line_applied)
			continue;
		dalog_rule_add(line);
		line_applied++;
	}

	fclose(fp);
	nmem_free_s(line);
}

static void* thread_monitor_cfgfile(void *user_data)
{
	char *path = (char*)user_data;
	int fd, wd, len, tmp_len;
	char buffer[2048], *offset = NULL;
	struct inotify_event *event;

	fd = inotify_init();
	if (fd < 0)
		goto quit;

	FILE *fp = fopen(path, "a+");
	if (fp)
		fclose(fp);

	wd = inotify_add_watch(fd, path, IN_MODIFY);
	if (wd < 0)
		goto quit;

	while ((len = read(fd, buffer, sizeof(buffer)))) {
		offset = buffer;
		event = (struct inotify_event*)(void*)buffer;

		while (((char*)event - buffer) < len) {
			if (event->wd == wd) {
				if (IN_MODIFY & event->mask)
					apply_rtcfg(path);
				break;
			}

			tmp_len = sizeof(struct inotify_event) + event->len;
			event = (struct inotify_event*)(void*)(offset + tmp_len);
			offset += tmp_len;
		}
	}

quit:
	free((void*)path);
	return NULL;
}

static void process_cfg(int argc, char *argv[])
{
	char *cfg;
	int i;

	/*
	 * 0. Load from .dalog.cfg
	 */
	load_cfg_file("~/.dalog.cfg");
	load_cfg_file("./.dalog.cfg");

	/*
	 * 1. Default configure file
	 */
	/* Load configure from env, DeFault ConFiGure */
	cfg = getenv("DALOG_DFCFG");
	if (cfg)
		load_cfg_file(cfg);

	/* Load configure from command line */
	i = narg_find(argc, argv, "--dalog-dfcfg", 1);
	if (i > 0)
		load_cfg_file(argv[i + 1]);

	/*
	 * 2. Runtime configure file
	 */
	cfg = NULL;
	i = narg_find(argc, argv, "--dalog-rtcfg", 1);
	if (i > 0)
		cfg = argv[i + 1];

	if (!cfg)
		cfg = getenv("DALOG_RTCFG");
	if (!cfg)
		cfg = "/tmp/dalog.rtcfg";
	if (cfg) {
		pthread_t thread;
		pthread_create(&thread, NULL, thread_monitor_cfgfile, strdup(cfg));
	}
}

static void rule_add_from_mask(unsigned int mask)
{
	char rule[256];
	int i;

	strcpy(rule, "mask=");
	i = 5;

	if (mask & DALOG_FATAL)
		rule[i++] = 'f';
	if (mask & DALOG_ALERT)
		rule[i++] = 'a';
	if (mask & DALOG_CRIT)
		rule[i++] = 'c';
	if (mask & DALOG_ERR)
		rule[i++] = 'e';
	if (mask & DALOG_WARNING)
		rule[i++] = 'w';
	if (mask & DALOG_NOTICE)
		rule[i++] = 'n';
	if (mask & DALOG_INFO)
		rule[i++] = 'i';
	if (mask & DALOG_DEBUG)
		rule[i++] = 'd';

	if (mask & DALOG_RTM)
		rule[i++] = 's';
	if (mask & DALOG_ATM)
		rule[i++] = 'S';

	if (mask & DALOG_PID)
		rule[i++] = 'j';
	if (mask & DALOG_TID)
		rule[i++] = 'x';

	if (mask & DALOG_LINE)
		rule[i++] = 'N';
	if (mask & DALOG_FILE)
		rule[i++] = 'F';
	if (mask & DALOG_MODU)
		rule[i++] = 'M';
	if (mask & DALOG_FUNC)
		rule[i++] = 'H';
	if (mask & DALOG_PROG)
		rule[i++] = 'P';

	rule[i++] = '\0';
	dalog_rule_add(rule);
}

void dalog_set_default_mask(unsigned int mask)
{
	__dft_mask = mask;
}

static void dalog_cleanup()
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i;

	for (i = 0; i < cc->arr_file_name.cnt; i++)
		nmem_free_s((void*)cc->arr_file_name.arr[i]);
	nmem_free_s((void*)cc->arr_file_name.arr);

	for (i = 0; i < cc->arr_modu_name.cnt; i++)
		nmem_free_s((void*)cc->arr_modu_name.arr[i]);
	nmem_free_s((void*)cc->arr_modu_name.arr);

	for (i = 0; i < cc->arr_prog_name.cnt; i++)
		nmem_free_s((void*)cc->arr_prog_name.arr[i]);
	nmem_free_s((void*)cc->arr_prog_name.arr);

	for (i = 0; i < cc->arr_func_name.cnt; i++)
		nmem_free_s((void*)cc->arr_func_name.arr[i]);
	nmem_free_s((void*)cc->arr_func_name.arr);

	nmem_free_s((void*)cc->arr_rule.arr);
}

void *dalog_init(int argc, char **argv)
{
	dalogcc_s *cc;

	if (__g_dalogcc)
		return (void*)__g_dalogcc;

	cc = (dalogcc_s*)nmem_alloz(1, dalogcc_s);
	__g_dalogcc = cc;

	pthread_mutex_init(&cc->mutex, 0);
	cc->pid = getpid();

	/* Set default before configure file */
	if (__dft_mask)
		rule_add_from_mask(__dft_mask);

	process_cfg(argc, argv);

	dalog_touch();

	atexit(dalog_cleanup);

	return (void*)__g_dalogcc;
}

int dalog_vf(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, va_list ap)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	va_list ap_copy0, ap_copy1;

	char buffer[4096], *bufptr = buffer;
	int i, ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long uptime = 0;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i]) {
			va_copy(ap_copy1, ap);
			cc->rloggers[i](type, mask, prog, modu, file, func, ln, fmt, ap_copy1);
			va_end(ap_copy1);
		}

	if (dalog_unlikely(!cc->nlogger_cnt))
		return 0;

	if (mask & (DALOG_RTM | DALOG_ATM))
		uptime = os_uptime();

	ofs = 0;

	/* Type */
	if (dalog_likely(type))
		ofs += sprintf(bufptr, "|%c|", type);

	/* Time */
	if (mask & DALOG_RTM)
		ofs += sprintf(bufptr + ofs, "s:%lu|", uptime);
	if (mask & DALOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "S:%s.%03d|", tmbuf, (unsigned int)(uptime % 1000));
	}

	/* ID */
	if (mask & DALOG_PID)
		ofs += sprintf(bufptr + ofs, "j:%d|", (int)cc->pid);
	if (mask & DALOG_TID)
		ofs += sprintf(bufptr + ofs, "x:%x|", (int)pthread_self());

	/* Name and LINE */
	if ((mask & DALOG_PROG) && prog)
		ofs += sprintf(bufptr + ofs, "P:%s|", prog);

	if ((mask & DALOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "M:%s|", modu);

	if ((mask & DALOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "F:%s|", file);

	if ((mask & DALOG_FUNC) && func)
		ofs += sprintf(bufptr + ofs, "H:%s|", func);

	if (mask & DALOG_LINE)
		ofs += sprintf(bufptr + ofs, "L:%d|", ln);

	if (dalog_likely(ofs))
		ofs += sprintf(bufptr + ofs, " ");

	va_copy(ap_copy0, ap);
	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap_copy0);
	va_end(ap_copy0);
	if (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		bufptr = nmem_alloc(bufsize, char);

		memcpy(bufptr, buffer, ofs);
		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}

	ret += ofs;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i])
			cc->nloggers[i](bufptr, ret);

	if (bufptr != buffer)
		nmem_free(bufptr);
	return ret;
}

int dalog_f(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = dalog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;
}

static int strarr_find(strarr_s *sa, char *str)
{
	int i;

	if (dalog_unlikely(!str))
		return -1;

	for (i = 0; i < sa->cnt; i++)
		if (sa->arr[i] && !strcmp(sa->arr[i], str))
			return i;
	return -1;
}

static char *strarr_add(strarr_s *sa, char *str)
{
	int pos = strarr_find(sa, str);

	if (pos != -1)
		return sa->arr[pos];

	if (sa->cnt >= sa->size)
		ARR_INC(256, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = strdup(str);
	sa->cnt++;

	/* Return the position been inserted */
	return sa->arr[sa->cnt - 1];
}

char *dalog_file_name_add(char *name)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	char *newstr, *newname = get_basename(name);

	pthread_mutex_lock(&cc->mutex);
	newstr = strarr_add(&cc->arr_file_name, newname);
	pthread_mutex_unlock(&cc->mutex);

	free(newname);

	return newstr;
}
char *dalog_modu_name_add(char *name)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	char *newstr;

	pthread_mutex_lock(&cc->mutex);
	newstr = strarr_add(&cc->arr_modu_name, name);
	pthread_mutex_unlock(&cc->mutex);
	return newstr;
}
char *dalog_prog_name_add(char *name)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	char *newstr, *newname, *progname;

	if (name)
		newname = get_basename(name);
	else {
		progname = get_progname();
		newname = get_basename(progname);
	}

	pthread_mutex_lock(&cc->mutex);
	newstr = strarr_add(&cc->arr_prog_name, newname);
	pthread_mutex_unlock(&cc->mutex);

	free(newname);

	return newstr;
}
char *dalog_func_name_add(char *name)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	char *newstr;

	pthread_mutex_lock(&cc->mutex);
	newstr = strarr_add(&cc->arr_func_name, name);
	pthread_mutex_unlock(&cc->mutex);
	return newstr;
}

static int rulearr_add(rulearr_s *ra, char *prog, char *modu,
		char *file, char *func, int line, int pid,
		unsigned int fset, unsigned int fclr)
{
	if (dalog_unlikely(ra->cnt >= ra->size))
		ARR_INC(16, ra->arr, ra->size, rule_s);

	rule_s *rule = &ra->arr[ra->cnt];

	rule->prog = prog;
	rule->modu = modu;
	rule->file = file;
	rule->func = func;

	rule->line = line;
	rule->pid = pid;

	rule->set = fset;
	rule->clr = fclr;

	ra->cnt++;

	/* Return the position been inserted */
	return ra->cnt - 1;
}

/*
 * rule =
 * prog=xxx,modu=xxx,file=xxx,func=xxx,line=xxx,pid=xxx,mask=left
 */
void dalog_rule_add(char *rule)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	int i_line, i_pid;
	char *s_prog, *s_modu, *s_file, *s_func, *s_line, *s_pid, *s_mask;
	char buf[1024];
	int i, blen;

	unsigned int set = 0, clr = 0;

	if (!rule || rule[0] == '#')
		return;

	buf[0] = ',';
	strncpy(buf + 1, rule, sizeof(buf) - 2);

	s_mask = strstr(buf, ",mask=");
	if (!s_mask || !s_mask[6])
		return;
	s_mask += 6;

	s_prog = strstr(buf, ",prog=");
	s_modu = strstr(buf, ",modu=");
	s_file = strstr(buf, ",file=");
	s_func = strstr(buf, ",func=");
	s_line = strstr(buf, ",line=");
	s_pid = strstr(buf, ",pid=");

	blen = strlen(buf);
	for (i = 0; i < blen; i++)
		if (buf[i] == ',')
			buf[i] = '\0';

	if (!s_prog || !s_prog[6])
		s_prog = NULL;
	else
		s_prog = dalog_prog_name_add(s_prog + 6);

	if (!s_modu || !s_modu[6])
		s_modu = NULL;
	else
		s_modu = dalog_modu_name_add(s_modu + 6);

	if (!s_file || !s_file[6])
		s_file = NULL;
	else
		s_file = dalog_file_name_add(s_file + 6);

	if (!s_func || !s_func[6])
		s_func = NULL;
	else
		s_func = dalog_func_name_add(s_func + 6);

	if (!s_line || !s_line[6])
		i_line = -1;
	else
		i_line = atoi(s_line + 6);

	if (!s_pid || !s_pid[5])
		i_pid = -1;
	else
		i_pid = atoi(s_pid + 6);

	dalog_parse_mask(s_mask, &set, &clr);

	if (set || clr) {
		pthread_mutex_lock(&cc->mutex);
		rulearr_add(&cc->arr_rule, s_prog, s_modu, s_file, s_func, i_line, i_pid, set, clr);
		pthread_mutex_unlock(&cc->mutex);

		dalog_touch();
	}
}
void dalog_rule_del(unsigned int idx)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();

	if (idx >= cc->arr_rule.cnt)
		return;

	memcpy(&cc->arr_rule.arr[idx], &cc->arr_rule.arr[idx + 1],
			(cc->arr_rule.cnt - idx - 1) * sizeof(rule_s));
	dalog_touch();
}
void dalog_rule_clr()
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();

	cc->arr_rule.cnt = 0;
	dalog_touch();
}

static unsigned int get_mask(char c)
{
	unsigned int i;

	static struct {
		char code;
		unsigned int bit;
	} flagmap[] = {
		{ '0', DALOG_FATAL },
		{ 'f', DALOG_FATAL },

		{ '1', DALOG_ALERT },
		{ 'a', DALOG_ALERT },

		{ '2', DALOG_CRIT },
		{ 'c', DALOG_CRIT },

		{ '3', DALOG_ERR },
		{ 'e', DALOG_ERR },

		{ '4', DALOG_WARNING },
		{ 'w', DALOG_WARNING },

		{ '5', DALOG_NOTICE },
		{ 'n', DALOG_NOTICE },

		{ '6', DALOG_INFO },
		{ 'i', DALOG_INFO },
		{ 'l', DALOG_INFO },

		{ '7', DALOG_DEBUG },
		{ 'd', DALOG_DEBUG },
		{ 't', DALOG_DEBUG },

		{ 's', DALOG_RTM },
		{ 'S', DALOG_ATM },

		{ 'j', DALOG_PID },
		{ 'x', DALOG_TID },

		{ 'N', DALOG_LINE },
		{ 'F', DALOG_FILE },
		{ 'M', DALOG_MODU },
		{ 'H', DALOG_FUNC },
		{ 'P', DALOG_PROG },
	};

	for (i = 0; i < sizeof(flagmap) / sizeof(flagmap[0]); i++)
		if (flagmap[i].code == c)
			return flagmap[i].bit;
	return 0;
}

static void dalog_parse_mask(char *mask, unsigned int *set, unsigned int *clr)
{
	int i = 0, len = strlen(mask);
	char c;

	*set = 0;
	*clr = 0;

	for (i = 0; i < len; i++) {
		c = mask[i];
		if (c == '-') {
			c = mask[++i];
			*clr |= get_mask(c);
		} else
			*set |= get_mask(c);
	}
}

unsigned int dalog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	dalogcc_s *cc = (dalogcc_s*)dalog_cc();
	unsigned int i, all = 0;
	int pid = (int)cc->pid;

	for (i = 0; i < cc->arr_rule.cnt; i++) {
		rule_s *rule = &cc->arr_rule.arr[i];

		if (rule->prog != NULL && rule->prog != prog)
			continue;
		if (rule->modu != NULL && rule->modu != modu)
			continue;
		if (rule->file != NULL && rule->file != file)
			continue;
		if (rule->func != NULL && rule->func != func)
			continue;
		if (rule->line != -1 && rule->line != line)
			continue;
		if (rule->pid != -1 && rule->pid != pid)
			continue;

		nflg_clr(all, rule->clr);
		nflg_set(all, rule->set);
	}

	return all;
}

