
f = open("mirror.cpp", "wt")
for i in xrange(0, 64):
    x = i & 7
    y = 7 - (i >>3)
    j = x | (y << 3)
    f.write("%d, " % j)
    if x == 7:
        f.write("\n")
f.close()