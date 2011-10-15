import os, os.path, string, re

f = open("peval.cpp")
lines = f.readlines()
f.close()

f = open("peval1.cpp", "wt")
for i, line in enumerate(lines):
    ro = re.compile('(([-]?[0-9]+)([,]?)([\s]*))', re.I)
    s = ro.search(line)
    if s and i > 1:
        line = (ro.sub(lambda m: '%4d%s' % (int(int(m.groups()[1])/2), m.groups()[2]), line) + "\n").lstrip()
        line = ' ' * (8-len(line.split(',')[0])) + line
    f.write(line)

f.close()    
    