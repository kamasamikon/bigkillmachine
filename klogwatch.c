/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/*-----------------------------------------------------------------------
 * Settings.
 *
 * 1) /proc/cmdline = klwtyp=<type=(local|remote)> klwopt=<filename|port>
 */

/*-----------------------------------------------------------------------
 * Local & Remote
 *
 * nc remote-ipaddr
 * nc localhost
 *
 * cmd = s | g
 * s klog-fa=sdfasdf
 * g           << return current value
 */
// 这里是景泰的配置，
klog.c::load_config()
{
    char *static_cfg = GetKLogStaticConfiguareUrl("/proc/cmdline");

    if (!static_cfg) {
        wget static_cfg -O "/tmp/klog_boot_parameter"
        static_cfg = "/tmp/klog_boot_parameter"
    } else
        static_cfg = "/etc/klog_boot_parameter"

    grep "/usr/local/bin/shit" /etc/klog/config;
    set the klog-fa and klog-ff
}

// 这里是动态的，动态的要等系统启动后才可以用
klog.c::watch_thread()
{
    int s = socket();
    unsigned short port = 9000 + getpid();

    // TODO: copy opt-server
    start_server(port);
    {
        dispath() {
            klog_dirty++;
        }
    }
}

static int __g_boot_argc;
static char **__g_boot_argv;

klog()
{
	static int first = 1;
	if (first) {
		load_boot_args();

		klog_init(__g_boot_argc, __g_boot_argv);
		opt_init(__g_boot_argc, __g_boot_argv);
		opt_rpc_server_init(__g_boot_argc, __g_boot_argv);
