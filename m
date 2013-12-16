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

recfg_list = []
rebuild_list = []

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

def findprjdir(basedir):
    if os.path.exists(basedir):
        output = subprocess.Popen(["find", basedir, "-name", "configure.ac"], stdout=subprocess.PIPE).stdout.readlines()
    else:
        output = []

    full_list = []
    for i in output:
        full_list.append(os.path.dirname(i.strip()) + '/')

    return set(full_list)

def update(odir, bdir, force_reconfig):
    odir_full = os.path.join(otv_rootdir, odir)
    bdir_full = os.path.join(otv_builddir, bdir)

    print "\n\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    # print odir_full
    print bdir_full

    dirty_files = syncdir(odir_full, bdir_full, True)
    if not dirty_files:
        return
    print "\nDirty files (DRY RUN)"
    for l in list(dirty_files):
        # print l
        pass

    prjdirs = findprjdir(bdir_full)
    if prjdirs:
        print "\nConfigure.ac DIR"
    for l in list(prjdirs):
        print l
        pass
    
    dirty_prjdirs = dirty_files & prjdirs
    if dirty_prjdirs:
        print "\nDirty projects"
    for l in list(dirty_prjdirs):
        print l
        pass

    for dirty_dir in list(dirty_prjdirs):
        recfg = False
        rebuild = False

        for path in list(dirty_files):
            if path.startswith(dirty_dir):
                if force_reconfig:
                    recfg = True
                    rebuild = False
                    break

                basename = os.path.basename(path)
                if basename in [ "configure.ac", "Makefile.am" ]:
                    recfg = True
                    rebuild = False
                    break
                else:
                    rebuild = True

        if recfg:
            print "\nReconfigure: %s" % dirty_dir
            recfg_list.append(os.path.basename(dirty_dir[:-1]))
            os.system("find '%s' -name '.configured' 2> /dev/null | xargs rm 2> /dev/null" % dirty_dir)
            syncdir(odir_full, bdir_full, False)
        elif rebuild:
            print "\nRebuild: %s" % dirty_dir
            rebuild_list.append(os.path.basename(dirty_dir[:-1]))
            os.system("find '%s' -newer '%s.configured' -a -type f 2> /dev/null | grep -v .deps | xargs rm 2> /dev/null" % (dirty_dir, dirty_dir))
            syncdir(odir_full, bdir_full, False)

def show_info(ct, bt):
    print ["CONFIG_TYPE not set, run 'nmk ct YOURTYPE' please", "CONFIG_TYPE=%s" % ct][ct != None]
    print ["BUILD_TYPE not set, run 'nmk bt YOURTYPE' please", "BUILD_TYPE=%s" % bt][bt != None]

def do_update():
    omodu = ".nmk.omodu"
    if not os.path.exists(omodu):
        omodu = os.path.join(bkm_rootdir, "nmk.omodu")
        if not os.path.exists(omodu):
            print "Error: ./.nmk.omodu or %s must be set, quit." % omodu
            sys.exit(0)

    # force re-configure
    force_reconfig = "-frc" in sys.argv
    start = timeit.default_timer()
    # Read list
    for line in open(omodu).readlines():
        # NTV Dir and Build Dir
        try:
            odir, bdir = line.strip().split(" ")
            update(odir, bdir, force_reconfig)
        except:
            pass

    if recfg_list:
        print "\nWill reconfigure:",
        for l in recfg_list:
            print l,
        print

    if rebuild_list:
        print "\nWill rebuit:",
        for l in rebuild_list:
            print l,
        print

    end = timeit.default_timer()
    print "\nTime cost %f" % (end - start)
    sys.exit(0)

def unset_grep_options():
    if os.environ.has_key("GREP_OPTIONS"):
        del os.environ["GREP_OPTIONS"]

def do_make():
    unset_grep_options()

    start = timeit.default_timer()
    os.chdir(otv_makedir)

    what = "source ../set_env_bash.sh; make CONFIG_TYPE=%s BUILD_TYPE=%s DEBUG_INIT=1 V=1 %s" % (ct, bt, " ".join(sys.argv[varindex:]))
    print what
    os.system(what)
    end = timeit.default_timer()
    print "Time cost %f" % (end - start)
    sys.exit(0)

def mark_rebuilt(dirpath):
    pass

def mark_reconfigure(dirpath):
    pass

def copy(src, dst):
    os.system("mkdir -p '%s'" % os.path.dirname(dst))
    os.system("cp -frv '%s' '%s'" % (src, dst))

def patch_dbus(bkm_7231dir):
    if os.path.exists(os.path.join(otv_rootdir, "buildroot/package/dbus/dbus-1.4.16-dispatch-hook.patch")):
        print "patch_dbus: already done, skip"
        return 

    # When patch dbus, the old one MUST be rebuilt
    os.system("rm -fr '%s'" % otv_builddir + "/dbus-1.4.16")
    copy(bkm_7231dir + "/dbus-1.4.16-dispatch-hook.patch", otv_rootdir + "/buildroot/package/dbus/")

def patch_ntvlog():
    ntvlog_path = otv_rootdir + "/nemotv/src/utils/ntvlog.h"
    if os.path.exists(ntvlog_path + ".nmbak"):
        print "patch_ntvlog: already done, skip"
        return

    copy(ntvlog_path, ntvlog_path + ".nmbak")
    copy(bkm_7231dir + "/ntvlog.h", otv_rootdir + "/nemotv/src/utils/")

    mark_rebuilt(otv_builddir + "/utils")
 
# Modify buildroot/Makefile and add -lhilda to it
def patch_buildroot_makefile(bkm_7231dir):
    if os.system("grep \"^O_BUILD_CFLAGS += -lhilda\" '%s/buildroot/Makefile'" % otv_rootdir):
        os.system("meld '%s' '%s'" % (bkm_7231dir + "/br-Makefile", otv_rootdir + "/buildroot/Makefile"))

def copy_bkm_build_files():
    # Copy to /staging/usr/lib and /staging/usr/include

    # hilda lib and inc
    copy(bkm_7231dir + "/libhilda.so", otv_stagedir + "/usr/lib/")
    copy(bkm_7231dir + "/hilda", otv_stagedir + "/usr/include/")

    patch_ntvlog()
    patch_dbus(bkm_7231dir)
    patch_buildroot_makefile(bkm_7231dir)

def copy_bkm_runtime_files():
    copy(bkm_7231dir + "/libhilda.so", otv_targetdir + "/target/usr/lib/")
    copy(bkm_7231dir + "/libnemohook.so", otv_targetdir + "/target/usr/lib/")
    copy(bkm_7231dir + "/klagent", otv_targetdir + "/target/usr/bin/")

def copy_bkm_files():
    copy_bkm_build_files()
    copy_bkm_runtime_files()

def set_dirs():
    global otv_makedir, otv_rootdir, otv_targetdir, otv_builddir, otv_stagedir, otv_imagedir, bkm_rootdir, bkm_7231dir

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc
    otv_makedir = os.getcwd()

    # ~/vvvvv/
    otv_rootdir = os.path.abspath(os.path.join(otv_makedir, "..", ".."))

    # ~/vvvvv/ntvtgt/sam7231_uclibc_bc/bld_sh_hdi/
    otv_targetdir = os.path.join(otv_makedir, "%s_%s" % (bt, ct))

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
    if os.system("diff '%s' '%s' &> /dev/null" % (bkm_7231dir + "/pcd", otv_targetdir + "/target/usr/sbin/pcd")):
        copy(bkm_7231dir + "/pcd", otv_targetdir + "/target/usr/sbin/")
    copy(otv_builddir + "/pcd-1.1.3/bin/target/usr/sbin/pcd", otv_targetdir + "/target/usr/sbin/pcd.real")

class EventHandler(pyinotify.ProcessEvent):
    def __init__(self, daemon, wm):
        self.daemon = daemon;
        self.wm = wm

    def process_IN_CREATE(self, event):
        # Replace PCD to pcd.real
        replace_pcd()

    def process_IN_MODIFY(self, event):
        replace_pcd()

class WatchThread(threading.Thread):
    def __init__(self, name, daemon):
        threading.Thread.__init__(self, name=name)
        self.daemon = daemon

        self.wm = pyinotify.WatchManager()
        self.notifier = pyinotify.AsyncNotifier(self.wm, EventHandler(daemon, self.wm))

        self.process_pcd_file()

    def run(self):
        asyncore.loop()

    def process_pcd_file(self):
        mask = pyinotify.IN_CREATE | pyinotify.IN_MODIFY
        sbin_path = otv_targetdir + "/target/usr/sbin"
        os.system("mkdir -p '%s'" % sbin_path)
        self.wm.add_watch(sbin_path, mask, rec=False)

# m --ct ConfigType --bt BuildType -i info -r|--restore 
if __name__ == "__main__":
    up = False
    go = False
    print_info = False

    varindex = 1
    i = 0
    argc = len(sys.argv)
    while i < argc:
        if sys.argv[i] == 'info':
            print_info = True
            break

        if sys.argv[i] == 'ct':
            i += 1
            if i < argc:
                os.system('echo "%s" > .nmk.ct' % sys.argv[i])
            else:
                print "Need CONFIG_TYPE"
                sys.exit(0)

        if sys.argv[i] == 'bt':
            i += 1
            if i < argc:
                os.system('echo "%s" > .nmk.bt' % sys.argv[i])
            else:
                print "Need BUILD_TYPE"
                sys.exit(0)

        if sys.argv[i] == 'up':
            up = True

        if sys.argv[i] == 'go':
            go = True
            varindex = i + 1

        i += 1

    ###
    # Ensure CONFIG_TYPE and BUILD_TYPE must be defined
    ###
    try:
        ct = open(".nmk.ct").readline().strip()
    except:
        ct = None

    try:
        bt = open(".nmk.bt").readline().strip()
    except:
        bt = None

    if print_info:
        show_info(ct, bt)
        sys.exit(0)

    if not ct or ct == "":
        print "CONFIG_TYPE not set, run 'nmk ct YOURTYPE' please"
        sys.exit(0)

    if not bt or bt == "":
        print "BUILD_TYPE not set, run 'nmk bt YOURTYPE' please"
        sys.exit(0)

    set_dirs()

    ###
    # Update the source
    ###
    if up:
        do_update()

    wt = WatchThread("WatchThread", True)
    wt.start()

    ###
    # Do make
    ###
    if argc == 1 or go:
        copy_bkm_files()
        do_make()

# After install, the header files and lib will be installed to staging/usr/local/include|lib

#
# Prepare - Environment
#   - Build
#       - Add -lhilda to Makefile
#       - Patch dbus
#       - Change ntvlog.h to new one
#       - Copy libhilda.so
#       - Copy inc/hilda
#   - Runtime
#       - libnemohook.so
#       - PCD
#           - mv pcd pcd.real
#           - cp bkm_pcd pcd
#
# Prepare - CheckUpdate
#   - rsync to build directory
#   - mark rebuild or reconfigure
#
# Make
# 

# name:cpfrdir:cptodir:belongto:dependby
# name = ID/name of a *DIRECTORY*
# cpfrdir = Copy/Sync from directory name
# cptodir = Copy/Sync to directory name
# belongto = name;name A name list 
# dependby = name;name A name list 

# name : configman
# cpfrdir : nemotv/src/configman
# cptodir : configman
# dependby : aim;recoder;otvgst

# vim: sw=4 ts=4 sts=4 ai et

