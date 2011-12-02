bits = 352192864402

for i in xrange(0, 64):
    if (i & 7) == 0 and i > 0:
        print ""
    if (bits & (1 << i)) != 0:
        print 1,
    else:
        print 0,
 