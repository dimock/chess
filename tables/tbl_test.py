import os, os.path, subprocess, thread

def run_engine():
    ename = r'D:\Projects\qt_proj\chess\tables\tequilla.exe'
    sp = subprocess.Popen( ename, shell = False, stdout = subprocess.PIPE, stdin = subprocess.PIPE)
    return sp

f = open(r'D:\Projects\qt_proj\chess\tables\book.book')
## f = open(r'D:\Projects\qt_proj\chess\tables\debut.tbl')
lines = f.readlines()
f.close()

for i, line in enumerate(lines):
    sp = run_engine()
    print >> sp.stdin, "new"
    print >> sp.stdin, "force"
    for cmd in line.split():
        print >> sp.stdin, cmd
    print >> sp.stdin, "quit"
    str = sp.communicate()[0]
    n = str.find("Illegal move")
    if n >= 0:
        print i, " - ", line
    #else:
	#	print '"'+line+'"', " passed OK"

print "finished"
