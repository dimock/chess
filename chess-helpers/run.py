from subprocess import Popen, PIPE
import subprocess

p = Popen("shallow.x64.exe", stdin = PIPE, stdout = PIPE, shell = True)

if not p:
	exit()
	
p.stdin.write('uci\n')
while True:
	line = p.stdout.readline()
	if not line:
		break;
	print line[:-2]
	if line[:-2] == 'uciok':
		break

print 'new game'
p.stdin.write('ucinewgame\n')
p.stdin.write('position startpos\n')
p.stdin.write('go movetime 15000\n')

score = 0
depth = 0
while True:
	line = p.stdout.readline()
	if not line:
		break
	tokens = line.split()
	if len(tokens) > 1 and tokens[0] == 'bestmove':
		print 'best move', tokens[1]
		break
	elif len(tokens) > 7:
		if tokens[5] == 'score':
			score = int(tokens[7])
		if tokens[1] == 'depth':
			d = int(tokens[2])
			if d > depth:
				depth = d
				print 'depth', depth
		
p.stdin.write('quit\n')

print 'score', score