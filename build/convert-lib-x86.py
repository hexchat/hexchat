# script by Mikhail Titov
# http://mail.gnome.org/archives/gtk-list/2011-March/msg00103.html

import os,re,sys,shutil
from os.path import join, getsize
from subprocess import Popen, PIPE
os.environ['PATH'] = os.environ['PATH'] + ";C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\Common7\\IDE;C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin;C:\\mozilla-build\\mingw32\\bin"
#gendef = "C:\\workspace\\glibmm-2.27.99\\MSVC_Net2008\\gendef\\Win32\\Debug\\gendef.exe"
#    dll = re.sub(".a", "", lib)
#    output = Popen([gendef, d, dll, lib], stdout=PIPE).communicate()[0]

def gen(dll):
    name = re.sub("^lib", "", dll)
    name = re.sub("(?:-\\d).dll", "", name)
#    shutil.copyfile(lib, name + ".lib")
    print("Working on %s\n" % dll)
    output = Popen(["nm", "lib%s.dll.a" % name], stdout=PIPE).communicate()[0]
    d = "%s.def" % name
    with open(d, "wb") as f:
        f.write(b"EXPORTS\n")
        for line in output.split(b"\r\n"):
            if (re.match(b".* T _|.* I __nm", line)): #|.* I __imp
                line = re.sub(b"^.* T _|^.* I __nm__", b"", line) #|^.* I _
                f.write(line + b"\n")
        f.write(str.encode("LIBRARY %s\n" % dll))
    p = Popen(["lib", "/machine:x86", "/def:%s" % d]) #, shell = True)

root = "c:\\mozilla-build\\build\\xchat-wdk\\dep-x86"
os.chdir(root + "\\lib")
for root, dirs, files in os.walk(root + "\\bin"):
    for name in files:
        if (re.search(".dll", name)):
            print("Processing: %s\n" % name)
            gen(name)

#gen("libatk-1.0-0.dll")
#  glibmm-2.4.def libglibmm-2.4-1.dll libglibmm-2.4.dll.a
# dumpbin /SYMBOLS /OUT:dumpbin.out libglibmm-2.4.dll.a
