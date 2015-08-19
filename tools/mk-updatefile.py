#!/usr/bin/env python

# 1) Please run this use python3.
# 2) Auto eject usb drive not supported, please eject it by hand.

import os, urllib.request, hashlib

url = "http://nemoPC:8000/otv5_loader_kernel_squashfs.bin"
blDir = "C:\\Users\\neyu\\Desktop\\StarHub_GX-SH530CF_Bootloader_18092013"
upanDir = "D:\\"

def httpGet(url, saveas):
    print("Get squashfs.bin from %s" % url)

    fp = urllib.request.urlopen(url)
    of = open(saveas, "wb")
    m = hashlib.md5()

    while True:
        data = fp.read(8192)
        if not data:
            break
        of.write(data)
        m.update(data)
    of.close()
    print("Checksum for otv5_loader_kernel_squashfs is %s" % m.hexdigest())

def update_update_version():
    print("Update version number in config3.txt and cfg.txt")

    config3_path = os.path.join(blDir, "Tools", "USBHeader_starhub", "config3.txt")
    cfg_path = os.path.join(blDir, "Tools", "SHB_Stream_Gen", "cfg.txt")

    config3_file = open(config3_path, "rt")
    config3_lines = config3_file.readlines()
    for line in config3_lines:
        if line.startswith("DOWNLOAD_SOFTWARE_VERSION"):
            config3_ver = int(line.split("=")[1].strip(), 16)
            break
    config3_file.close()

    cfg_file = open(cfg_path, "rt")
    cfg_lines = cfg_file.readlines()
    for line in cfg_lines:
        if line.startswith("NEW_VERSION_DEC"):
            cfg_ver = int(line.split("=")[1].strip(), 10)
            break
    cfg_file.close()

    print("config3_ver = %d" % config3_ver)
    print("cfg_ver = %d" % cfg_ver)

    next_ver = max(config3_ver, cfg_ver) + 1
    print("NEXT VERSION = %d" % next_ver)

    config3_file = open(config3_path, "wt")
    for line in config3_lines:
        if line.startswith("DOWNLOAD_SOFTWARE_VERSION"):
            config3_file.write("DOWNLOAD_SOFTWARE_VERSION = 0x%08x\n" % next_ver)
        else:
            config3_file.write(line)
    config3_file.close()

    cfg_file = open(cfg_path, "wt")
    for line in cfg_lines:
        if line.startswith("NEW_VERSION_DEC"):
            cfg_file.write("NEW_VERSION_DEC=%d\n" % next_ver)
        else:
            cfg_file.write(line)
    cfg_file.close()


if __name__ == "__main__":

    ### Get IMAGE file
    saveas = os.path.join(blDir, "Tools", "otv5_loader_kernel_squashfs.bin")
    httpGet(url, saveas)
    os.system("pause")

    ### Update the version number
    update_update_version()
    os.system("pause")

    ### Call BAT file
    print("Call download_file_generater.bat")
    os.chdir(os.path.join(blDir, "Tools"))
    os.system("download_file_generater.bat")

    ### Copy output file to USB Drive
    print("Copy the GX-SH530CF-usb.bin to USB Drive")
    outputfile = os.path.join(blDir, "Tools", "output", "GX-SH530CF-usb.bin")
    os.system("copy %s %s" % (outputfile, upanDir))

    os.system("pause")

