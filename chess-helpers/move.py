import os, os.path, re

while True:
    m = raw_input()
    if m.lower().find('q') >= 0:
        break
    if len(m) == 2 and m[0].isalpha():
        x = ord(m[0]) - ord('a')
        y = ord(m[1]) - ord('1')
        print x+y*8
    elif len(m) > 0 and m[0].isdigit():
        i = int(m)
        x = i & 7
        y = i >> 3
        print chr(ord('a') + x) + str(y+1)