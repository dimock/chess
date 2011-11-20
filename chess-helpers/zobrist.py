import random

f = open("zobrist.cpp", "wt")
num = 1341243
for j in range(0, 4):
    for i in range(0, 64):
        r = random.randint(1000000, 99999999)
        num = num ^ (r << i)
    f.write( "0x%X, " % (num & 0xFFFFFFFFFFFFFFFF) )
f.close()
print num
