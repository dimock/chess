import os, sys, os.path, re

if len(sys.argv) < 2:
    exit()

base_ver = '1.0.0'

def readVer():
    f = os.popen('git log -1 --format="%h"')
    if not f:
        return base_ver
    lines = f.readlines()
    f.close()
    if len(lines) < 1:
        return base_ver
    gitcode = lines[0][0:-1]
    str = re.sub('([\\d][.][\\d][.])([\\d])', lambda z: z.groups()[0] + gitcode, base_ver)
    return str

    
outname = sys.argv[1]
name, ext = os.path.splitext(outname)
version = readVer()
name, platform = os.path.splitext(name)
newname = name + '.' + version + platform + ext
print 'rename engine to', newname
if os.path.isfile(newname):
	os.remove(newname)
os.rename(outname, newname)

