mask = 0
for i in range(0, 64):
    x = i & 7
    if x != 7:
        mask |= 1 << i
  
print "%x" % mask