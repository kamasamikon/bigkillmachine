/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "NHIOCTL"
#include <dalog/dalog.h>

#define MAX_DMP_SIZE 128

#define NH_IOCTL

/*-----------------------------------------------------------------------
 * ioctl - PI
 */
#ifdef NH_IOCTL
#include <sys/ioctl.h>

typedef enum
{
	NH_DEV_ID_AUDDEC,
	NH_DEV_ID_CRYPTO,
	NH_DEV_ID_DMX,
	NH_DEV_ID_FPCHAR,
	NH_DEV_ID_HDMI,
	NH_DEV_ID_INJECT,
	NH_DEV_ID_LED,
	NH_DEV_ID_LINKER,
	NH_DEV_ID_NOCS,
	NH_DEV_ID_PD_WRITER,
	NH_DEV_ID_RFMOD,
	NH_DEV_ID_SCART,
	NH_DEV_ID_SMARTCARD,
	NH_DEV_ID_STB,
	NH_DEV_ID_TUNER,
	NH_DEV_ID_VIDDEC,
	NH_DEV_ID_VIDENC,
	NH_DEV_ID_SOC,
	NH_DEV_ID_AUDOUT,
	NH_DEV_ID_FAN,
	NH_DEV_ID_TRANSCODE
} nh_dev_id_t;

struct dev_name_map {
	nh_dev_id_t id;
	const char *name;
} __g_devs[] = {
	{ NH_DEV_ID_AUDDEC, "PIDEV_AUDDEC" },
	{ NH_DEV_ID_CRYPTO, "PIDEV_CRYPTO" },
	{ NH_DEV_ID_DMX, "PIDEV_DMX" },
	{ NH_DEV_ID_FPCHAR, "PIDEV_FPCHAR" },
	{ NH_DEV_ID_HDMI, "PIDEV_HDMI" },
	{ NH_DEV_ID_INJECT, "PIDEV_INJECT" },
	{ NH_DEV_ID_LED, "PIDEV_LED" },
	{ NH_DEV_ID_LINKER, "PIDEV_LINKER" },
	{ NH_DEV_ID_NOCS, "PIDEV_NOCS" },
	{ NH_DEV_ID_PD_WRITER, "PIDEV_PD_WRITER" },
	{ NH_DEV_ID_RFMOD, "PIDEV_RFMOD" },
	{ NH_DEV_ID_SCART, "PIDEV_SCART" },
	{ NH_DEV_ID_SMARTCARD, "PIDEV_SMARTCARD" },
	{ NH_DEV_ID_STB, "PIDEV_STB" },
	{ NH_DEV_ID_TUNER, "PIDEV_TUNER" },
	{ NH_DEV_ID_VIDDEC, "PIDEV_VIDDEC" },
	{ NH_DEV_ID_VIDENC, "PIDEV_VIDENC" },
	{ NH_DEV_ID_SOC, "PIDEV_SOC" },
	{ NH_DEV_ID_AUDOUT, "PIDEV_AUDOUT" },
	{ NH_DEV_ID_FAN, "PIDEV_FAN" },
	{ NH_DEV_ID_TRANSCODE, "PIDEV_TRANSCODE" },
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
	static int skip_klog = -1;
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

	if (dagou_unlikely(skip_klog == -1)) {
		if (getenv("NH_IOCTL_SKIP"))
			skip_klog = 1;
		else
			skip_klog = 0;
	}
	if (dagou_unlikely(skip_klog))
		return ret;

	unsigned int i, cnt, dir = _IOC_DIR(r), type = _IOC_TYPE(r), size = _IOC_SIZE(r);

	switch (type) {
	case NH_DEV_ID_AUDDEC:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_CRYPTO:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_DMX:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_FPCHAR:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_HDMI:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_INJECT:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_LED:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_LINKER:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_NOCS:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_PD_WRITER:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_RFMOD:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_SCART:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_SMARTCARD:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_STB:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_TUNER:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_VIDDEC:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_VIDENC:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_SOC:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_AUDOUT:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_FAN:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	case NH_DEV_ID_TRANSCODE:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	default:
		klog("ioctl: d:%d r:%08x dir:%s dev:%s nr:%02x size:%d arg:%08x.\n",
				d, r, dir_name(dir), dev_name(type), _IOC_NR(r), _IOC_SIZE(r), argp);
		break;
	}
	return ret;
}
#endif
