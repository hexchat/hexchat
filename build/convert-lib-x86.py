# a2lib script by Mikhail Titov
# http://live.gnome.org/GTK%2B/Win32/NativeBuildWithOBS

import os,re,sys,shutil
from os.path import join, getsize
from subprocess import Popen, PIPE
os.environ['PATH'] = os.environ['PATH'] + ";C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\Common7\\IDE;C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin;C:\\mozilla-build\\mingw32\\bin"
root = "c:\\mozilla-build\\build\\xchat-wdk\\dep-x86"

def gen(dll,lib,d):
    output = Popen(["nm", lib], stdout=PIPE).communicate()[0]
    with open(d, "wb") as f:
        f.write(b"EXPORTS\n")
        for line in output.split(b"\r\n"):
            if (re.match(b".* T _|.* I __nm", line)): #|.* I __imp
                line = re.sub(b"^.* T _|^.* I __nm__", b"", line) #|^.* I _
                f.write(line + b"\n")
        f.write(str.encode("LIBRARY %s\n" % dll))
    p = Popen(["lib", "/MACHINE:X86", "/def:%s" % d]) #, shell = True)

os.chdir(root + "\\lib")
for root, dirs, files in os.walk(root + "\\bin"):
    for f in files:
        if (re.search(".dll", f)):
            name = re.sub("^lib", "", f)
            name = re.sub("(?:-\\d).dll", "", name)
            print("Working on %s\n" % f)
            d = "%s.def" % name
            lib = "lib%s.dll.a" % name
            gen(f, lib, d)
