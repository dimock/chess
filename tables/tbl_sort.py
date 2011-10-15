import os, os.path

def sorttbl(fname):
    f = open(fname, 'rt')
    lines = f.readlines()
    f.close()
    lines.sort()
    f = open(fname, 'wt')
    for line in lines:
        f.write(line)
    f.close()

sorttbl("debut.tbl")
