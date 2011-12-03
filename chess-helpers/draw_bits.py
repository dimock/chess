import sys

if len(sys.argv) < 2:
    print "give bit-mask"
    exit(0)

bits = long(sys.argv[1])

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
 