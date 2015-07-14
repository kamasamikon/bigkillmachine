/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <hilda/klog.h>
#include <hilda/karg.h>
#include <hilda/kopt.h>
#include <hilda/kopt-rpc-server.h>

static int os_rlog_rule(int ses, void *opt, void *pa, void *pb)
{
	char *cfgfile = (char*)kopt_ua(opt);
	char *rule = kopt_get_new_str(opt);
	char cmd[2048];

	sprintf(cmd, "echo '%s' >> '%s'", rule, cfgfile);
	system(cmd);
	return 0;
}

int main(int argc, char *argv[])
{
	kopt_init(argc, argv);

	kopt_add("s:/rlog/rule", NULL, OA_DFT, os_rlog_rule, NULL, NULL, argv[1], NULL);

	kopt_add_s("b:/sys/admin/nemo/enable", OA_GET, NULL, NULL);
	kopt_add_s("s:/sys/usr/nemo/passwd", OA_GET, NULL, NULL);

	kopt_setint("b:/sys/admin/nemo/enable", 1);
	kopt_setstr("s:/sys/usr/nemo/passwd", "nemo");

	kopt_rpc_server_init(9900, argc, argv);

	while (1)
		pause();
}

