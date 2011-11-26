
f = open("bits.cpp", "wt")
for byte in xrange(0, 256):
    if (byte > 0) and (byte % 16) == 0:
        f.write("\n")
    n = 0
    while byte != 0:
        if (byte & 1) != 0:
            n += 1
        byte >>= 1        
    f.write("%d, " % (n))
f.close()