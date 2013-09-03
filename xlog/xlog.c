/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/*
 * FILE_NAME
 * MODU_NAME
 * PROC_ID
 * THRD_ID
 */

static int file_nr, modu_nr, prog_nr;

#define SET_FILE_NR() do {
	if (file_nr != -1) {
		file_nm = fmt_file_name(__FILE__);
		file_nr = todo;
	}
} while (0);

#define klog(fmt, ...) do {
	/* �Ƿ����óظ��� */
	static int ver_sav = -1;
	int ver_get = klog_cfg_ver();

	/* �Ƿ���ʾ */
	static char showme = false;

	/* flag �������Ƿ���ʾ */
	int flg;

	if (ver_get < ver_sav) {
		/* ���óظ����ˣ������µİ汾�� */
		ver_sav = ver_get;
		SET_FILE_NR();
		SET_MODU_NR();
		SET_PROG_NR();

		int proc_id, thrd_id, line;
		flg = klog_calc_flg(file_name, modu_name, proc_id, thrd_id, line);

		showme = ???;
	}

	if (showme) {
		global path_name = str_from_nr(prog_nr);
		global file_name = str_from_nr(file_nr);
		/* �������ļ����Ǹ�ʽ������ */
		klogf('L', flg, KLOG_MODU, file_name, __LINE__, fmt, ##__VA_ARGS__);
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

typedef struct _strarr_t strarr_t;
struct _strarr_t {
	int size;
	int cnt;
	char **arr;
};

int strarr_add(strarr_t sa, const char *str)
{
	if (sa->cnt >= sa->size)
		ARR_INC(10, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = strdup(str);
	sa->cnt++;
}
int strarr_find(strarr_t sa, const char *str)
{
	int i;

	for (i = 0; i < sa->cnt; i++)
		if (!strcmp(sa->arr[i], cmdname))
			return i;
	return -1;
}
// �����ַ���һ���б�NR������������
int cmdname_to_nr(char *str)
{
	return strarr_find(cmd_sa, str);
}

int modname_to_nr(char *cmdname)
{
	return strarr_find(cmd_sa, str);
}

static int file_nr, modu_nr, prog_nr;
int klog_calc_flg(int prog_nr, int modu_nr, int pid, int tid, int func_nr, int file_nr)
{
	// fname = NULL => ignore
	// mname = NULL => ignore
	// pid = -1 => ignore
	// tid = -1 => ignore
	// line = -1 => ignore
	//
	// ���л�������
	// PID, TID, ������

	// |����|ģ��|PID|TID|file|func|line=left
	// /usr/bin/shit|shit.c|SHIT|3423|45234|455=leftAM

	/* shit.c �ĵ�100�� */
	// NULL|shit.c|NULL|3423|0|100=leftAM

	// ��ʼ����ʱ�������о��Ѿ��õ���
	// �ļ���Ӧ�ø�ʽ��һ�£�

	for (int i = 0; i < MAX; i++) {
		if (fname)
			if (fname == rule[i].name)
				shit;
	}
}

