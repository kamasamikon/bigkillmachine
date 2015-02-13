/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "NHIOCTL"
#include <dalog.h>

#define MAX_DMP_SIZE 128

#define DAGOU_IOCTL

/*-----------------------------------------------------------------------
 * ioctl - PI
 */
#ifdef DAGOU_IOCTL
#include <sys/ioctl.h>

typedef enum
{
	DAGOU_DEV_ID_AUDDEC,
	DAGOU_DEV_ID_CRYPTO,
	DAGOU_DEV_ID_DMX,
	DAGOU_DEV_ID_FPCHAR,
	DAGOU_DEV_ID_HDMI,
	DAGOU_DEV_ID_INJECT,
	DAGOU_DEV_ID_LED,
	DAGOU_DEV_ID_LINKER,
	DAGOU_DEV_ID_NOCS,
	DAGOU_DEV_ID_PD_WRITER,
	DAGOU_DEV_ID_RFMOD,
	DAGOU_DEV_ID_SCART,
	DAGOU_DEV_ID_SMARTCARD,
	DAGOU_DEV_ID_STB,
	DAGOU_DEV_ID_TUNER,
	DAGOU_DEV_ID_VIDDEC,
	DAGOU_DEV_ID_VIDENC,
	DAGOU_DEV_ID_SOC,
	DAGOU_DEV_ID_AUDOUT,
	DAGOU_DEV_ID_FAN,
	DAGOU_DEV_ID_TRANSCODE
} DAGOU_dev_id_t;

struct dev_name_map {
	DAGOU_dev_id_t id;
	const char *name;
} __g_devs[] = {
	{ DAGOU_DEV_ID_AUDDEC, "PIDEV_AUDDEC" },
	{ DAGOU_DEV_ID_CRYPTO, "PIDEV_CRYPTO" },
	{ DAGOU_DEV_ID_DMX, "PIDEV_DMX" },
	{ DAGOU_DEV_ID_FPCHAR, "PIDEV_FPCHAR" },
	{ DAGOU_DEV_ID_HDMI, "PIDEV_HDMI" },
	{ DAGOU_DEV_ID_INJECT, "PIDEV_INJECT" },
	{ DAGOU_DEV_ID_LED, "PIDEV_LED" },
	{ DAGOU_DEV_ID_LINKER, "PIDEV_LINKER" },
	{ DAGOU_DEV_ID_NOCS, "PIDEV_NOCS" },
	{ DAGOU_DEV_ID_PD_WRITER, "PIDEV_PD_WRITER" },
	{ DAGOU_DEV_ID_RFMOD, "PIDEV_RFMOD" },
	{ DAGOU_DEV_ID_SCART, "PIDEV_SCART" },
	{ DAGOU_DEV_ID_SMARTCARD, "PIDEV_SMARTCARD" },
	{ DAGOU_DEV_ID_STB, "PIDEV_STB" },
	{ DAGOU_DEV_ID_TUNER, "PIDEV_TUNER" },
	{ DAGOU_DEV_ID_VIDDEC, "PIDEV_VIDDEC" },
	{ DAGOU_DEV_ID_VIDENC, "PIDEV_VIDENC" },
	{ DAGOU_DEV_ID_SOC, "PIDEV_SOC" },
	{ DAGOU_DEV_ID_AUDOUT, "PIDEV_AUDOUT" },
	{ DAGOU_DEV_ID_FAN, "PIDEV_FAN" },
	{ DAGOU_DEV_ID_TRANSCODE, "PIDEV_TRANSCODE" },
};

static const char* dev_name(int id)
{
	int i;
	static char name[8];

	for (i = 0; i < sizeof(__g_devs) / sizeof(__g_devs[0]); i++)
		if (__g_devs[i].id == id)
			return __g_devs[i].name;
	sprintf(name, "?%x?", id);
	return name;
}

static const char *dir_name(unsigned int dir)
{
	static char name[8];

	if (dir == _IOC_NONE)
		return "N";
	else if (dir == (_IOC_WRITE | _IOC_READ))
		return "RW";
	else if (dir == _IOC_WRITE)
		return "W";
	else if (dir == _IOC_READ)
		return "R";

	sprintf(name, "?%x?", dir);
	return name;
}

int ioctl(int d, unsigned long int r, ...)
{
	static int skip_dalog = -1;
	static int (*realfunc)(int d, unsigned long int r, void*) = NULL;
	va_list args;
	void *argp;

	if (dagou_unlikely(!realfunc))
		realfunc = dlsym(RTLD_NEXT, "ioctl");

	va_start(args, r);
	argp = va_arg(args, void *);
	va_end(args);

	dalog_setup();
	int ret = realfunc(d, r, argp);

	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("DAGOU_IOCTL_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	if (dagou_unlikely(skip_dalog))
		return ret;

	unsigned int i, cnt, dir = _IOC_DIR(r), type = _IOC_TYPE(r), size = _IOC_SIZE(r);

	switch (type) {
	case DAGOU_DEV_ID_AUDDEC:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_CRYPTO:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_DMX:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_FPCHAR:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_HDMI:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_INJECT:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_LED:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_LINKER:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_NOCS:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_PD_WRITER:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_RFMOD:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_SCART:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_SMARTCARD:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_STB:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_TUNER:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_VIDDEC:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_VIDENC:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_SOC:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_AUDOUT:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_FAN:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case DAGOU_DEV_ID_TRANSCODE:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	default:
		dalog_info("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	}
	return ret;
}
#endif

