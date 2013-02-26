import sys, string, re

if len(sys.argv) < 2:
    print "give bit-mask"
    exit(0)

radix = 10
if ( re.compile("(0x)([0-9abcdef]+)", re.I).search(sys.argv[1]) != None ):
    radix = 16
    
bits = long(sys.argv[1], radix)

for i in xrange(0, 64):
    x = i & 7
    y = i >>3
    j = x | ((7-y) <<3)
    if (i & 7) == 0 and i > 0:
        print ""
    if (bits & (1 << j)) != 0:
        print 1,
    else:
        print 0,
 