/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/*
 * FILE_NAME
 * MODU_NAME
 * PROC_ID
 * THRD_ID
 */

#define klog(fmt, ...) do {
	/* 是否配置池更新 */
	static int ver_sav = -1;
	int ver_get = klog_cfg_ver();

	/* 是否显示 */
	static char showme = false;

	/* flag 决定了是否显示 */
	int flg;

	if (ver_get < ver_sav) {
		/* 配置池更新了，保存新的版本号 */
		ver_sav = ver_get;

		char *file_name, *modu_name;
		int proc_id, thrd_id, line;
		flg = klog_calc_flg(file_name, modu_name, proc_id, thrd_id, line);

		showme = ???;
	}

	if (showme) {
		klogf('L', flg, KLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
	}
} while (0)

int klog_cfg_ver()
{
	return 2;
}

struct _name_arr_t {
	char *name;
	int nr;
};

typedef struct _rule_t rule_t;
struct _rule_t {
	short path;		/* Process command line */
	short module;		/* Module name */

	int pid;		/* Process ID */
	int tid;		/* Thread ID */

	int file;		/* File name */
	int func;		/* Func name */
	int line;		/* Line number */
};

typedef struct _str_arr_t str_arr_t;
struct _str_arr_t {
	int size;
	int cnt;
	char **arr;
};

int strs_add(_str_arr_t sa, const char *str)
{
	if (sa->cnt >= sa->size)
		ARR_INC(10, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = strdup(str);
	sa->cnt++;
}
int strs_find(_str_arr_t sa, const char *str)
{
	int i;

	for (i = 0; i < sa->cnt; i++)
		if (!strcmp(sa->arr[i], cmdname))
			return i;
	return -1;
}
// 把名字放入一个列表，NR就是他的索引
int cmdname_to_nr(char *str)
{
	return strs_find(cmd_sa, str);
}

int modname_to_nr(char *cmdname)
{
	return strs_find(cmd_sa, str);
}

int klog_calc_flg(const char *fname, const char *mname, int pid, int tid, int line)
{
	// fname = NULL => ignore
	// mname = NULL => ignore
	// pid = -1 => ignore
	// tid = -1 => ignore
	// line = -1 => ignore
	//
	// 运行环境部分
	// PID, TID, 命令行

	|程序|模块|PID|TID|file|func|line=left
	/usr/bin/shit|shit.c|SHIT|3423|45234|455=leftAM

	/* shit.c 的第100行 */
	NULL|shit.c|NULL|3423|0|100=leftAM

	// 初始化的时候，命令行就已经拿到了
	// 文件名应该格式化一下，

	for (int i = 0; i < MAX; i++) {
		if (fname)
			if (fname == rule[i].name)
				shit;
	}
}

