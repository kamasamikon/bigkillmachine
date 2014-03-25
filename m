#!/usr/bin/env python

import os, sys, subprocess, timeit
import asyncore, threading, pyinotify

import time

### ###########################################################
## TARGETS
#

# ~/vvvvv/ntvtgt/sam7231_uclibc_bc
otv_makedir = None

# ~/vvvvv/
otv_rootdir = None

# ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/
otv_targetdir = None

# ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/build/
otv_builddir = None

# ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/staging/
otv_stagedir = None

# ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/images/
otv_imagedir = None

# ~/eoe/auv/bigkillmachine/
bkm_rootdir = None

# ~/eoe/auv/bigkillmachine/build/target/7231/
bkm_7231dir = None

config_type = None
build_type = None
build_mode = None

modu_attrs = {}

### ###########################################################
## MISC HELPER
#
s_days = 60 * 60 * 24
s_hours = 60 * 60
s_minutes = 60

def fmtTime(t):
    s = ""

    if t >= s_days:
        s += "%d days " % (t / s_days)

    t = t % s_days
    if t >= s_hours:
        s += "%d hours " % (t / s_hours)

    t = t % s_hours
    if t >= s_minutes:
        s += "%d minutes " % (t / s_minutes)

    t = t % s_minutes
    s += "%d seconds" % t

    return s

def print_color(line, cr="red"):
    if cr == "red":
        print "\033[1;31m%s\033[0m" % line
    elif cr == "yellow":
        print "\033[1;33m%s\033[0m" % line
    else:
        print line

def show_cost_time(t):
    print_color("Time cost %f or %s" % (t, fmtTime(t)))

### ###########################################################
## 
#

def shell_run(cmd):
    if build_mode:
        os.environ["BKM_BM"] = build_mode
    if config_type:
        os.environ["BKM_CT"] = config_type
    if build_type:
        os.environ["BKM_BT"] = build_type
    return os.system(cmd)

def syncdir(frdir, todir, dryrun):
    if not os.path.exists(frdir):
        return None

    if dryrun:
        switch = "-avn"
    else:
        switch = "-av"

    output = subprocess.Popen(["rsync", switch, frdir + "/", todir], stdout=subprocess.PIPE).stdout.readlines()

    full_list = []
    for i in output[1:-3]:
        full_list.append(todir + '/' + i.strip())

    skip_list = [ "tags", ".git" ]

    if full_list:
        full_list.append(todir + '/')

    return set(full_list).difference(skip_list)

def update_attr(attrs):
    # XXX: Only set attrs related directories

    cpfrdir = attrs["cpfrdir"]
    cptodir = attrs["cptodir"]

    dirty_files = syncdir(cpfrdir, cptodir, True)
    if not dirty_files:
        return

    for path in list(dirty_files):
        basename = os.path.basename(path)
        if basename in [ "configure.ac", "Makefile.am" ]:
            attrs["reconfigure"] = True
            attrs["rebuild"] = True # fasten process_relation()
            break
        else:
            attrs["rebuild"] = True

def get_omodu_path():
    omodu = ".nmk.omodu"
    if not os.path.exists(omodu):
        omodu = os.path.join(bkm_rootdir, "nmk.omodu")
        if not os.path.exists(omodu):
            print_color("Error: ./.nmk.omodu or %s must be set, quit." % omodu)
            sys.exit(0)
    return omodu

def load_omodu_attrs(omodu_path):
    for line in open(omodu_path).readlines():
        try:
            line = line.strip()
            if not line or line[0] == '#':
                continue

            attrs = {}
            for seg in line.split(" "):
                k,v = seg.split("=")
                attrs[k] = v

            if not attrs.has_key("cpfrdir"):
                print_color("No modu name set")
                continue

            if not attrs.has_key("name"):
                attrs["name"] = os.path.basename(attrs["cpfrdir"])

            if attrs.has_key("dependon"):
                attrs["dependon"] = attrs["dependon"].split(":")
            else:
                attrs["dependon"] = []

            attrs["cpfrdir"] = os.path.join(otv_rootdir, attrs["cpfrdir"])

            if not attrs.has_key("cptodir"):
                attrs["cptodir"] = os.path.basename(attrs["cpfrdir"])
            attrs["cptodir"] = os.path.join(otv_builddir, attrs["cptodir"])

            attrs["reconfigure"] = False
            attrs["rebuild"] = False

            #print attrs
            modu_attrs[attrs["name"]] = attrs
        except:
            pass

def process_relation():
    def do_it():
        processed = 0
        for dummy,attrs in modu_attrs.items():
            if attrs["reconfigure"] == True:

                for dependon in attrs["dependon"]:
                    if modu_attrs[dependon]["reconfigure"] == False:
                        modu_attrs[dependon]["reconfigure"] = True
                        processed += 1

            elif attrs["rebuild"] == True:

                for dependon in attrs["dependon"]:
                    if modu_attrs[dependon]["rebuild"] == False:
                        modu_attrs[dependon]["rebuild"] = True
                        processed += 1
        return processed

    while do_it():
        pass

def sync_files():
    if "--dryrun" in sys.argv:
        dryrun = True
    else:
        dryrun = False

    for dummy,attrs in modu_attrs.items():
        if attrs["reconfigure"] == True:
            print_color("reconfigure: %s" % attrs["name"])
            shell_run("find '%s' -name '.configured' 2> /dev/null | xargs rm 2> /dev/null" % attrs["cptodir"])
            syncdir(attrs["cpfrdir"], attrs["cptodir"], dryrun)
        elif attrs["rebuild"] == True:
            print_color("rebuild: %s" % attrs["name"])
            shell_run("find '%s' -newer '%s.configured' -a -type f 2> /dev/null | grep -v .deps | xargs rm 2> /dev/null" % (attrs["cptodir"], attrs["cptodir"]))
            syncdir(attrs["cpfrdir"], attrs["cptodir"], dryrun)

def fill_omodu_attrs():
    omodu_path = get_omodu_path()
    load_omodu_attrs(omodu_path)

def update_all_omodu_attrs():
    for dummy,attrs in modu_attrs.items():
        update_attr(attrs)

def do_update(modus):
    start = timeit.default_timer()

    fill_omodu_attrs()
    if modus:
        for modu_name in modus:
            if modu_attrs.has_key(modu_name):
                update_attr(modu_attrs[modu_name])
            else:
                print_color("update: '%s' not found." % modu_name, "yellow")
    else:
        update_all_omodu_attrs()
    process_relation()
    sync_files()

    end = timeit.default_timer()
    show_cost_time(end - start)
    sys.exit(0)

def do_clean(modus):
    unset_grep_options()

    start = timeit.default_timer()
    os.chdir(otv_makedir)

    targets = ""
    for m in modus:
        targets += " %s-clean %s-dirclean " % (m, m)

    what = "source ../set_env_bash.sh; make CONFIG_TYPE=%s BUILD_TYPE=%s DEBUG_INIT=1 V=1 %s" % (config_type, build_type, targets)
    print_color(what, "yellow")
    shell_run(what)
    end = timeit.default_timer()
    show_cost_time(end - start)
    sys.exit(0)

def unset_grep_options():
    if os.environ.has_key("GREP_OPTIONS"):
        del os.environ["GREP_OPTIONS"]

def do_make():
    unset_grep_options()

    start = timeit.default_timer()
    os.chdir(otv_makedir)

    what = "source ../set_env_bash.sh; make CONFIG_TYPE=%s BUILD_TYPE=%s DEBUG_INIT=1 V=1 %s" % (config_type, build_type, " ".join(sys.argv[varindex:]))
    print_color(what, "yellow")
    shell_run(what)
    end = timeit.default_timer()
    show_cost_time(end - start)
    sys.exit(0)

def mark_rebuilt(modu_name):
    fill_omodu_attrs()
    if modu_attrs.has_key(modu_name):
        modu_attrs[modu_name]["rebuilt"] = True
        process_relation()
        sync_files()

def mark_reconfigure(modu_name):
    fill_omodu_attrs()
    if modu_attrs.has_key(modu_name):
        modu_attrs[modu_name]["reconfigure"] = True
        process_relation()
        sync_files()

def copy(src, dst):
    print_color("Copy [%s] => [%s]" % (src, dst), "yellow")
    shell_run("mkdir -p '%s'" % os.path.dirname(dst))
    shell_run("cp -fr '%s' '%s'" % (src, dst))

def is_diff(path0, path1):
    if shell_run("diff '%s' '%s' &> /dev/null" % (path0, path1)):
        return True
    return False

def patch_dbus(bkm_7231dir):
    bkm_dbus_patch_path = bkm_7231dir + "/dbus-1.4.16-dispatch-hook.patch"
    br_dbus_patch_path = otv_rootdir + "/buildroot/package/dbus/dbus-1.4.16-dispatch-hook.patch"

    if not is_diff(bkm_dbus_patch_path, br_dbus_patch_path):
        print_color("patch_dbus: already done, skip")
        return 

    # When patch dbus, the old one MUST be rebuilt
    shell_run("rm -fr '%s'" % otv_builddir + "/dbus-1.4.16")
    copy(bkm_dbus_patch_path, br_dbus_patch_path)

def process_ntvlog():
    # ntvlog.h.nmbak === Don's ntvlog.h
    ntvlog_path = otv_rootdir + "/nemotv/src/utils/ntvlog.h"
    ntvlog_nmbak_path = ntvlog_path + ".nmbak"
    ntvlog_bkm_path = bkm_7231dir + "/ntvlog.h"

    rebuild = False

    # 1. Ensure the nmbak file exists
    if not os.path.exists(ntvlog_nmbak_path):
        print_color("Backup Don's ntvlog.h to ntvlog.h.nmbak")
        copy(ntvlog_path, ntvlog_nmbak_path)

        if shell_run("grep '275 Sacramento Street' %s" % ntvlog_nmbak_path):
            print_color("==================================================")
            print_color("= Fatal error: ntvlog.h.nmbak is not don's version")
            print_color("==================================================")
            sys.exit(0)

    if build_mode == 'bkm':
        # PatchMode: the ntvlog.h must same as bkm version
        if is_diff(ntvlog_path, ntvlog_bkm_path):
            copy(ntvlog_bkm_path, ntvlog_path)
            rebuild = True

    elif build_mode == 'otv':
        # RestoreMode: the ntvlog.h must same as Don's version
        if is_diff(ntvlog_path, ntvlog_nmbak_path):
            copy(ntvlog_nmbak_path, ntvlog_path)
            rebuild = True

    if rebuild:
        shell_run("rm -fr %s/utils" % otv_builddir)
 
# Modify buildroot/Makefile and add -lnhlog to it
def patch_buildroot_makefile(bkm_7231dir):
    if shell_run("grep \"^O_BUILD_CFLAGS += -lnhlog\" '%s/buildroot/Makefile'" % otv_rootdir):
        shell_run("meld '%s' '%s'" % (bkm_7231dir + "/br-Makefile", otv_rootdir + "/buildroot/Makefile"))

def copy_nhlog_to_staging():
    copy(bkm_7231dir + "/libnhlog.so", otv_stagedir + "/usr/lib/")
    copy(bkm_7231dir + "/nhlog.h", otv_stagedir + "/usr/include/")

def copy_ntvlog2_to_staging():
    copy(bkm_7231dir + "/ntvlog2.h", otv_stagedir + "/usr/include/")

def copy_hilda_to_staging():
    if not os.path.isdir(otv_stagedir): # or not os.path.islink(otv_stagedir):
        print_color("%s is not exists" % otv_stagedir)
        return

    # -L and -I files
    copy(bkm_7231dir + "/libhilda.so", otv_stagedir + "/usr/lib/")
    copy(bkm_7231dir + "/hilda", otv_stagedir + "/usr/include/")

def copy_bkm_build_files():
    # Copy to /staging/usr/lib and /staging/usr/include

    copy_nhlog_to_staging()
    copy_ntvlog2_to_staging()
    copy_hilda_to_staging()

    patch_dbus(bkm_7231dir)
    patch_buildroot_makefile(bkm_7231dir)

    process_ntvlog()

def copy_bkm_runtime_files():
    copy(bkm_7231dir + "/libhilda.so", otv_targetdir + "/target/usr/lib/")
    copy(bkm_7231dir + "/libnemohook.so", otv_targetdir + "/target/usr/lib/")
    copy(bkm_7231dir + "/rtk", otv_targetdir + "/target/usr/bin/")
    copy(bkm_7231dir + "/klbench", otv_targetdir + "/target/usr/bin/")
    copy(bkm_7231dir + "/klagent", otv_targetdir + "/target/usr/bin/")
    copy(bkm_7231dir + "/klagent.sh", otv_targetdir + "/target/usr/bin/")
    copy(bkm_7231dir + "/ldmon", otv_targetdir + "/target/usr/bin/")
    copy(bkm_7231dir + "/libnhlog.so", otv_targetdir + "/target/usr/lib/")
    copy(bkm_7231dir + "/libkong.so", otv_targetdir + "/target/usr/lib/")

def set_build_info():
    hfile = open(otv_targetdir + "/target/BUILD.INFO", "wt")
    hfile.write("Build at: %s\n" % time.asctime(time.localtime(time.time())))
    hfile.write("Root: %s\n" % otv_targetdir)
    hfile.write("Mode: %s" % build_mode)
    hfile.close()

def copy_bkm_files():
    copy_bkm_build_files()
    copy_bkm_runtime_files()
    set_build_info()

def set_dirs():
    global otv_makedir, otv_rootdir, otv_targetdir, otv_builddir, otv_stagedir, otv_imagedir, bkm_rootdir, bkm_7231dir

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc
    otv_makedir = os.getcwd()

    # ~/vvvvv/
    otv_rootdir = os.path.abspath(os.path.join(otv_makedir, "..", ".."))

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/
    otv_targetdir = os.path.join(otv_makedir, "%s_%s" % (build_type, config_type))

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/build/
    otv_builddir = os.path.join(otv_targetdir, "build")

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/staging/
    otv_stagedir = os.path.join(otv_targetdir, "staging")

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/images/
    otv_imagedir = os.path.join(otv_targetdir, "images")

    # ~/eoe/auv/bigkillmachine/
    bkm_rootdir = os.path.dirname(os.path.realpath(__file__))

    # ~/eoe/auv/bigkillmachine/build/target/7231/
    bkm_7231dir = os.path.join(bkm_rootdir, "build", "target", "7231")

def replace_pcd():
    pcd_sbin_path = otv_targetdir + "/target/usr/sbin/pcd"
    pcd_real_sbin_path = otv_targetdir + "/target/usr/sbin/pcd.real"

    if is_diff(bkm_7231dir + "/pcd", pcd_sbin_path):
        copy(bkm_7231dir + "/pcd", pcd_sbin_path)

    if os.path.exists(otv_builddir + "/pcd-1.1.3/bin/target/usr/sbin/pcd"):
        copy(otv_builddir + "/pcd-1.1.3/bin/target/usr/sbin/pcd", pcd_real_sbin_path)

class EventHandler(pyinotify.ProcessEvent):
    def __init__(self, daemon, wm):
        self.daemon = daemon;
        self.wm = wm
        self.pcd_path = otv_targetdir + "/target/usr/sbin/pcd"

    def process_IN_CREATE(self, event):
        if event.pathname == self.pcd_path:
            print "process_IN_CREATE:" + event.pathname
            replace_pcd()
        elif event.pathname == otv_stagedir:
            print "process_IN_CREATE:" + event.pathname
            copy_hilda_to_staging()

    def process_IN_MODIFY(self, event):
        if event.pathname == self.pcd_path:
            print "process_IN_MODIFY:" + event.pathname
            replace_pcd()

class WatchThread(threading.Thread):
    def __init__(self, name, daemon):
        threading.Thread.__init__(self, name=name)
        self.daemon = daemon

        self.wm = pyinotify.WatchManager()
        self.notifier = pyinotify.AsyncNotifier(self.wm, EventHandler(daemon, self.wm))

        self.watch_pcd_file()
        self.watch_staging_dir()

    def run(self):
        asyncore.loop()

    def watch_pcd_file(self):
        sbin_path = otv_targetdir + "/target/usr/sbin"
        shell_run("mkdir -p '%s'" % sbin_path)

        mask = pyinotify.IN_CREATE | pyinotify.IN_MODIFY
        self.wm.add_watch(sbin_path, mask, rec=False)

    def watch_staging_dir(self):
        mask = pyinotify.IN_CREATE | pyinotify.IN_MODIFY
        print "watch_staging_dir: %s" % otv_targetdir
        self.wm.add_watch(otv_targetdir, mask, rec=False)

def show_info():
    print_color(["CONFIG_TYPE not set, run 'nmk ct YOURTYPE' please", "CONFIG_TYPE=%s" % config_type][config_type != None])
    print_color(["BUILD_TYPE not set, run 'nmk bt YOURTYPE' please", "BUILD_TYPE=%s" % build_type][build_type != None])
    print_color(["MODE not set, run 'nmk bm YOURTYPE' please", "MODE=%s" % build_mode][build_mode != None])

# m --ct ConfigType --bt BuildType -i info -r|--restore 
if __name__ == "__main__":
    up = False
    clean = False
    go = False
    print_info = False

    modus = None
    varindex = 1
    i = 0
    argc = len(sys.argv)
    while i < argc:
        if sys.argv[i] == 'info':
            print_info = True
            break

        if sys.argv[i] == 'bm':
            i += 1
            if i < argc:
                shell_run('echo "%s" > .nmk.bm' % sys.argv[i])
            else:
                print_color("No NMK MODE set, default is otv mode")
                shell_run('echo "%s" > .nmk.bm' % "otv")

        if sys.argv[i] == 'ct':
            i += 1
            if i < argc:
                shell_run('echo "%s" > .nmk.ct' % sys.argv[i])
            else:
                print_color("Need CONFIG_TYPE")
                sys.exit(0)

        if sys.argv[i] == 'bt':
            i += 1
            if i < argc:
                shell_run('echo "%s" > .nmk.bt' % sys.argv[i])
            else:
                print_color("Need BUILD_TYPE")
                sys.exit(0)

        if sys.argv[i] == 'up':
            up = True
            i += 1
            if i < argc:
                modus = sys.argv[i:]
            break

        if sys.argv[i] == 'clean':
            clean = True
            i += 1
            if i < argc:
                modus = sys.argv[i:]
            break

        if sys.argv[i] == 'go':
            go = True
            varindex = i + 1

        i += 1

    ###
    # Ensure CONFIG_TYPE and BUILD_TYPE must be defined
    ###
    try:
        config_type = open(".nmk.ct").readline().strip()
    except:
        config_type = None

    try:
        build_type = open(".nmk.bt").readline().strip()
    except:
        build_type = None

    try:
        build_mode = open(".nmk.bm").readline().strip()
    except:
        build_mode = None

    if print_info:
        show_info()
        sys.exit(0)

    if not config_type or config_type == "":
        print_color("CONFIG_TYPE not set, run 'nmk ct YOURTYPE' please")
        sys.exit(0)

    if not build_type or build_type == "":
        print_color("BUILD_TYPE not set, run 'nmk bt YOURTYPE' please")
        sys.exit(0)

    if build_mode not in [ "bkm", "otv" ]:
        print_color("BUILD_MODE not set, run 'nmk bm [bkm | otv]' please")
        sys.exit(0)

    set_dirs()

    ###
    # Update the source
    ###
    if up:
        do_update(modus)

    if clean:
        do_clean(modus)

    ###
    # Do make
    ###
    if argc == 1 or go:
        replace_pcd()
        wt = WatchThread("WatchThread", True)
        wt.start()

        copy_bkm_files()
        do_make()

# name:cpfrdir:cptodir:dependon
# name = ID/name of a *DIRECTORY*
# cpfrdir = Copy/Sync from directory name
# cptodir = Copy/Sync to directory name
# dependon = name;name A name list 

# name : configman
# cpfrdir : nemotv/src/configman
# cptodir : configman
# dependon : aim;recoder;otvgst

# vim: sw=4 ts=4 sts=4 ai et
