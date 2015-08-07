#! /usr/bin/env python3

from html.parser import HTMLParser
import os, sys, urllib.request, time, hashlib, socket
from ftplib import FTP
import subprocess

HTTP_HOST = "nemonb"
HTTP_PORT = 8000
HTTP_ADDR = "http://%s:%d/" % (HTTP_HOST, HTTP_PORT)

COMM_HOST = "nemonb"
COMM_PORT = 60006
COMM_SOCK = None

FTP_HOST = "nemonb"
FTP_USER = "ftp"
FTP_PASS = "ftp"

OUT_IMG_PATH = os.path.join("root", "sam_tools", "4unsigned", "GX-SH530CF-usb.bin")

def print2(msg):
    print(msg)

    if COMM_SOCK:
        try:
            COMM_SOCK.send(bytes("%s\n" % msg, "utf-8"))
        except:
            pass
    else:
        print("NO COMM_SOCK")

# create a subclass and override the handler methods
class MyHTMLParser(HTMLParser):
    def __init__(self):
        self.hrefs = []
        HTMLParser.__init__(self)

    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            for k,v in attrs:
                if k == 'href':
                    self.hrefs.append(v)

def get_file_info(url):
    c = urllib.request.urlopen(url)
    info = c.info()

    m = hashlib.md5()
    dat = c.read(4096)
    while dat:
        m.update(dat)
        dat = c.read(4096)

    return info.get("Content-Length", "-1"), info.get("Last-Modified", "-1"), m.hexdigest()

def get_file(url, altname = None, recursive = True):
    print2(">>> URL: '%s'" % url)
    c = urllib.request.urlopen(url)
    print2(c.info())

    isdir, name = get_name(url)
    if altname:
        name = altname

    if recursive and isdir:
        if not os.path.exists(name):
            os.mkdir(name)
        elif not os.path.isdir(name):
            print2(">>> Error: '%s' exist" % name)

        os.chdir(name)
        dat = c.read()
        for href in get_href_list(dat):
            get_file(url + href, None, recursive)
        os.chdir("..")
    else:
        f = open(name, "wb")
        dat = c.read(4096)
        while dat:
            f.write(dat)
            dat = c.read(4096)
        f.close()

def get_href_list(data):
    parser = MyHTMLParser()
    parser.feed(str(data))
    return parser.hrefs

def get_name(url):
    url = urllib.request.unquote(url)
    if url.endswith("/"):
        return True, os.path.basename(url[:-1])
    return False, os.path.basename(url)

def get_ipaddr():
    try:
        csock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        csock.connect(('8.8.8.8', 80))
        (addr, port) = csock.getsockname()
        csock.close()
        return addr
    except socket.error:
        return "127.0.0.1"

def calc_checksum(path):
    f = open(path, "rb")
    dat = f.read(4096)
    m = hashlib.md5()
    while dat:
        m.update(dat)
        dat = f.read(4096)
    f.close()
    return m.hexdigest()

def upload_gx_sh530cf_usb_bin(path):
    squashfs_bin_md5 = calc_checksum("otv5_loader_kernel_squashfs.bin")

    imgfile = "GX-SH530CF-usb.bin"
    remote_name = 'STOR i/%s-%s' % (imgfile, squashfs_bin_md5)

    # XXX: rename myfile.txt
    try:
        ftp = FTP(FTP_HOST, FTP_USER, FTP_PASS)
        ftp.storbinary(remote_name, open(path, 'rb'))
        ftp.close()
        print2(">>> UPLOAD '%s-%s' FINISHED." % (imgfile, squashfs_bin_md5))
    except KeyboardInterrupt:
        COMM_SOCK.close()
        sys.exit(0)
    except Exception as e:
        print2(">>> UPLOAD '%s-%s' FAILED." % (imgfile, squashfs_bin_md5))
        print2("------------------------------------")
        print2(e)
        print2("------------------------------------")
    print2(">>> QUIT FROM upload_gx_sh530cf_usb_bin\n\n\n")

def do_command(cmd):
    args = cmd.decode().split()

    print2(cmd)

    if not args:
        return

    if args[0] == "up":
        print2(">>> WILL MAKE IMAGE")
        get_file(HTTP_ADDR + "otv5_loader_kernel_squashfs.bin")

        os.chdir("root")
        os.chdir("sam_tools")
        os.chdir("4unsigned")

        proc = subprocess.Popen(['download_file_generater.bat'], stdout = subprocess.PIPE)
        while True:
            line = proc.stdout.readline()
            if line != b"":
                print2(line.decode().strip())
            else:
                break

        os.chdir("..");
        os.chdir("..");
        os.chdir("..");

        COMM_SOCK.send(bytes("download_file_generater.bat DONE\n", "utf-8"))

        if os.path.exists(OUT_IMG_PATH):
            upload_gx_sh530cf_usb_bin(OUT_IMG_PATH)
        else:
            print2(">>> ERROR: %s is not generated." % OUT_IMG_PATH)

    if args[0] == "again":
        print2(">>> WILL RE-GET ROOT")
        get_file(HTTP_ADDR, "root")

    if args[0] == "bye":
        sys.exit(0)

def main():
    global COMM_SOCK

    COMM_SOCK = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        COMM_SOCK.connect((COMM_HOST, COMM_PORT))
        COMM_SOCK.send(bytes("\n\n\n\n\nHEY, I IS FROM %s.\n" % get_ipaddr(), "utf-8"))

        get_file(HTTP_ADDR, "root")

        print2(">>> I IS WAITING COMMAND")
        while True:
            cmd = b""
            c = COMM_SOCK.recv(1)
            while c != b"\n":
                cmd += c
                c = COMM_SOCK.recv(1)
            do_command(cmd)
    except Exception as e:
        print2(e)
    except KeyboardInterrupt:
        COMM_SOCK.close()
        sys.exit(0)

    finally:
        COMM_SOCK.close()
        COMM_SOCK = None

if __name__ == "__main__":
    count = 0
    while True:
        print("Call main :%d" % count)
        main()
        time.sleep(1)
        count += 1

