/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <time.h>
#include <string.h>

#include <rlog.h>
#include <kflg.h>
#include <karg.h>
#include <kmem.h>

#ifndef CFG_RLOG_DO_NOTHING

/* Control Center for rlog */
typedef struct _rlogcc_t rlogcc_t;
struct _rlogcc_t {
	/** Global flags, set by rlog_init, default is no message */
	unsigned int flg;

	/** command line for application, seperate by '\0' */
	char ff[4096];

	/** touches is a ref count user change rlog arg */
	int touches;
};

static rlogcc_t *__g_rlogcc = NULL;

static void set_fa_arg(int argc, char **argv);
static void set_ff_arg(int argc, char **argv);

/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *rlog_attach(void *logcc)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();

	return (void*)cc;
}

kinline void *rlog_cc(void)
{
	/* XXX: If user don't call rlog_init, call it for them */
	if (__g_rlogcc)
		return (void*)__g_rlogcc;
	else
		return rlog_init(LOG_ALL, 0, NULL);
}

kinline void rlog_touch(void)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();

	cc->touches++;
}
kinline int rlog_touches(void)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();

	return cc->touches;
}

/* opt setting should has the same format as argv */
/* just like a raw command line */
void rlog_setflg(const char *cmd)
{
	int argc;
	char **argv;

	if ((argv = build_argv(cmd, &argc, &argv)) == NULL)
		return;

	set_fa_arg(argc, argv);
	set_ff_arg(argc, argv);
	rlog_touch();

	free_argv(argv);
}

static void set_fa_str(const char *arg)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();
	char c;
	const char *p = arg;

	while ((c = *p++)) {
		if (c == '-') {
			c = *p++;
			if (c == 'l')
				kflg_clr(cc->flg, LOG_LOG);
			else if (c == 'e')
				kflg_clr(cc->flg, LOG_ERR);
			else if (c == 'f')
				kflg_clr(cc->flg, LOG_FAT);
			else if (c == 't')
				kflg_clr(cc->flg, LOG_RTM);
			else if (c == 'T')
				kflg_clr(cc->flg, LOG_ATM);
			else if (c == 'L')
				kflg_clr(cc->flg, LOG_LINE);
			else if (c == 'F')
				kflg_clr(cc->flg, LOG_FILE);
			else if (c == 'M')
				kflg_clr(cc->flg, LOG_MODU);
			else if (c == 'i')
				kflg_clr(cc->flg, LOG_PID);
			else if (c == 'I')
				kflg_clr(cc->flg, LOG_TID);
		} else if (c == 'l')
			kflg_set(cc->flg, LOG_LOG);
		else if (c == 'e')
			kflg_set(cc->flg, LOG_ERR);
		else if (c == 'f')
			kflg_set(cc->flg, LOG_FAT);
		else if (c == 't')
			kflg_set(cc->flg, LOG_RTM);
		else if (c == 'T')
			kflg_set(cc->flg, LOG_ATM);
		else if (c == 'L')
			kflg_set(cc->flg, LOG_LINE);
		else if (c == 'F')
			kflg_set(cc->flg, LOG_FILE);
		else if (c == 'M')
			kflg_set(cc->flg, LOG_MODU);
		else if (c == 'i')
			kflg_set(cc->flg, LOG_PID);
		else if (c == 'I')
			kflg_set(cc->flg, LOG_TID);
	}
}

/* Flag for All files */
static void set_fa_arg(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc && argv[i]; i++)
		if (!strncmp(argv[i], "--rlog-fa=", 10))
			set_fa_str(argv[i] + 10);
}

/* Flag for Files */
static void set_ff_arg(int argc, char **argv)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();
	int i;
	char *p = cc->ff;

	/* XXX: tricky */
	strcat(p, " = ");

	for (i = 0; i < argc && argv[i]; i++)
		if (!strncmp(argv[i], "--rlog-ff=", 10)) {
			p = strcat(p, argv[i] + 10);
			p = strcat(p, " ");
		}
}

/**
 * \brief Set parameters for debug message, should be called once in main
 * --rlog-fa=lef-t --rlog-ff=<left>=:file1:file2:
 *
 * \param flg ored of LOG_LOG, LOG_ERR and LOG_FAT
 * \return Debug message flag after set.
 */
void *rlog_init(unsigned int flg, int argc, char **argv)
{
	rlogcc_t *cc;

	if (__g_rlogcc)
		return (void*)__g_rlogcc;

	cc = (rlogcc_t*)kmem_alloz(1, rlogcc_t);
	__g_rlogcc = cc;

	cc->flg = flg;

	set_fa_arg(argc, argv);
	set_ff_arg(argc, argv);
	rlog_touch();

	return (void*)__g_rlogcc;
}

/**
 * \brief Current debug message flag.
 *
 * This will check the command line to do more on each file.
 *
 * The command line format is --rlog-ff=<left> :file1.ext:file2.ext:file:
 *      file.ext is the file name. If exist dup name file, the flag set to all.
 */
unsigned int rlog_getflg(const char *file)
{
	rlogcc_t *cc = (rlogcc_t*)rlog_cc();
	char pattern[256], *start;
	kbool set;
	unsigned int flg = cc->flg;

	sprintf(pattern, ":%s:", file);

	start = strstr(cc->ff, pattern);
	if (start) {
		while (*start-- != '=')
			;

		while (*start != ' ') {
			if (*(start - 1) == '-')
				set = kfalse;
			else
				set = ktrue;

			switch(*start) {
			case 'l':
				if (set)
					kflg_set(flg, LOG_LOG);
				else
					kflg_clr(flg, LOG_LOG);
				break;
			case 'e':
				if (set)
					kflg_set(flg, LOG_ERR);
				else
					kflg_clr(flg, LOG_ERR);
				break;
			case 'f':
				if (set)
					kflg_set(flg, LOG_FAT);
				else
					kflg_clr(flg, LOG_FAT);
				break;
			case 't':
				if (set)
					kflg_set(flg, LOG_RTM);
				else
					kflg_clr(flg, LOG_RTM);
				break;
			case 'T':
				if (set)
					kflg_set(flg, LOG_ATM);
				else
					kflg_clr(flg, LOG_ATM);
				break;
			case 'N':
				if (set)
					kflg_set(flg, LOG_LINE);
				else
					kflg_clr(flg, LOG_LINE);
				break;
			case 'F':
				if (set)
					kflg_set(flg, LOG_FILE);
				else
					kflg_clr(flg, LOG_FILE);
				break;
			case 'M':
				if (set)
					kflg_set(flg, LOG_MODU);
				else
					kflg_clr(flg, LOG_MODU);
				break;
			default:
				break;
			}

			if (set)
				start--;
			else
				start -= 2;
		}
	}

	return flg;
}

int rlogf(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, ...)
{
	va_list ap;
	char buffer[2048], *bufptr = buffer;
	int ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick = 0;

	va_start(ap, fmt);

	rlog_v(type, flg, modu, file, ln, fmt, ap);

	if (flg & (LOG_RTM | LOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;
	if (type)
		ofs += sprintf(bufptr, "|%c|", type);
	if (flg & LOG_RTM)
		ofs += sprintf(bufptr + ofs, "%lu|", tick);
	if (flg & LOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (unsigned int)(tick % 1000));
	}
	if (flg & LOG_PID)
		ofs += sprintf(bufptr + ofs, "%d|", (int)spl_process_currrent());
	if (flg & LOG_TID)
		ofs += sprintf(bufptr + ofs, "%x|", (int)spl_thread_current());
	if ((flg & LOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "%s|", modu);
	if ((flg & LOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "%s|", file);
	if (flg & LOG_LINE)
		ofs += sprintf(bufptr + ofs, "%d|", ln);
	if (ofs)
		ofs += sprintf(bufptr + ofs, " ");

	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	while (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);

		ofs = 0;
		if (type)
			ofs += sprintf(bufptr, "|%c|", type);
		if (flg & LOG_RTM)
			ofs += sprintf(bufptr + ofs, "%lu|", tick);
		if (flg & LOG_ATM)
			ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (unsigned int)(tick % 1000));
		if (flg & LOG_PID)
			ofs += sprintf(bufptr + ofs, "%d|", (int)spl_process_currrent());
		if (flg & LOG_TID)
			ofs += sprintf(bufptr + ofs, "%x|", (int)spl_thread_current());
		if ((flg & LOG_MODU) && modu)
			ofs += sprintf(bufptr + ofs, "%s|", modu);
		if ((flg & LOG_FILE) && file)
			ofs += sprintf(bufptr + ofs, "%s|", file);
		if (flg & LOG_LINE)
			ofs += sprintf(bufptr + ofs, "%d|", ln);
		if (ofs)
			ofs += sprintf(bufptr + ofs, " ");

		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}
	va_end(ap);

	ret += ofs;
	rlog_s(bufptr, ret);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;
}

#endif /* CFG_RLOG_DO_NOTHING */
