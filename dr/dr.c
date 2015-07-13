/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define SZ 2048

static void help()
{
	printf("drs switches\n");
	printf("switches:\n");
	printf("    Prog: prog=??? P:??? p:???\n");
	printf("    Modu: modu=??? M:??? m:???\n");
	printf("    File: file=??? F:??? f:??? ???.[c|cpp|cxx]\n");
	printf("    Func: func=??? H:??? h:???\n");
	printf("    Line: line=??? L:??? l:??? <digital>\n");
	printf("    Mask: mask=??? ALL -ALL or [^PMFHL]\n");
	printf("\n");
	printf("Masks:\n");
	printf("    f=fatal a=alert c=critial e=error\n");
	printf("    w=warning i=info n=notice d=debug\n");
	printf("\n");
	printf("Marks:\n");
	printf("    P=prog M=modu F=file H=func N=line\n");
	printf("    s=RTM S=ATM j=PID x=TID\n");
}

int main(int argc, char *argv[])
{
	int i;

	int argl;
	char *args;

	static char prog[SZ], modu[SZ], file[SZ], func[SZ], line[SZ], mask[SZ];
	static char cfgline[SZ];

	FILE *fp;

	for (i = 1; i < argc; i++) {
		args = argv[i];
		argl = strlen(args);

		if (!strcmp("--help", args)) {
			help();
			exit(1);


		} else if (!strncmp("prog=", args, 5)) {
			strcpy(prog, &args[5]);
		} else if (!strncmp("P:", args, 2)) {
			strcpy(prog, &args[2]);
		} else if (!strncmp("p:", args, 2)) {
			strcpy(prog, &args[2]);


		} else if (!strncmp("modu=", args, 5)) {
			strcpy(modu, &args[5]);
		} else if (!strncmp("M:", args, 2)) {
			strcpy(modu, &args[2]);
		} else if (!strncmp("m:", args, 2)) {
			strcpy(modu, &args[2]);


		} else if (!strncmp("file=", args, 5)) {
			strcpy(file, &args[5]);
		} else if (!strncmp("F:", args, 2)) {
			strcpy(file, &args[2]);
		} else if (!strncmp("f:", args, 2)) {
			strcpy(file, &args[2]);


		} else if (!strncmp("func=", args, 5)) {
			strcpy(func, &args[5]);
		} else if (!strncmp("H:", args, 2)) {
			strcpy(func, &args[2]);
		} else if (!strncmp("h:", args, 2)) {
			strcpy(func, &args[2]);


		} else if (!strncmp("line=", args, 5)) {
			strcpy(line, &args[5]);
		} else if (!strncmp("L:", args, 2)) {
			strcpy(line, &args[2]);
		} else if (!strncmp("l:", args, 2)) {
			strcpy(line, &args[2]);


		} else if (!strncmp("mask=", args, 5)) {
			strcat(mask, &args[5]);


		} else if ((argl > 2) && !strncmp(".c", &args[argl - 2], 2)) {
			strcpy(file, args);
		} else if ((argl > 4) && !strncmp(".cpp", &args[argl - 4], 4)) {
			strcpy(file, args);
		} else if ((argl > 4) && !strncmp(".cxx", &args[argl - 4], 4)) {
			strcpy(file, args);


		} else if (!strcmp("ALL", args)) {
			strcat(mask, "facewind");
		} else if (!strcmp("-ALL", args)) {
			strcat(mask, "-f-a-c-e-w-i-n-d");


		} else if (args[0] >= '0' && args[0] <= '9') {
			errno = 0;
			strtoul(args, NULL, 10);
			if (errno == 0)
				strcpy(line, args);
		} else {
			strcat(mask, args);
		}
	}

	if (!mask[0]) {
		help();
		exit(1);
	}

	if (prog[0]) {
		strcat(cfgline, "prog=");
		strcat(cfgline, prog);
		strcat(cfgline, ",");
	}

	if (modu[0]) {
		strcat(cfgline, "modu=");
		strcat(cfgline, modu);
		strcat(cfgline, ",");
	}

	if (file[0]) {
		strcat(cfgline, "file=");
		strcat(cfgline, file);
		strcat(cfgline, ",");
	}

	if (func[0]) {
		strcat(cfgline, "func=");
		strcat(cfgline, func);
		strcat(cfgline, ",");
	}

	if (line[0]) {
		strcat(cfgline, "line=");
		strcat(cfgline, line);
		strcat(cfgline, ",");
	}

	strcat(cfgline, "mask=");
	strcat(cfgline, mask);

	printf("%s\n", cfgline);

	fp = fopen("/tmp/dalog.rtcfg", "a");
	if (!fp) {
		printf("Open /tmp/dalog.rtcfg failed, errno: %d\n", errno);
	}

	fprintf(fp, "%s\n", cfgline);

	return 0;
}

