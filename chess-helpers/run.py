from subprocess import Popen, PIPE
import subprocess, msvcrt, time
from win32pipe import PeekNamedPipe


engines = ["D:\\Projects\\git_proj\\chess\\x64\\release\\shallow.x64.exe", "D:\\arena_3.0\\Engines\\ALChess1.5b\\ALChess1.5b.exe"]
##"D:\\Projects\\git_proj\\chess\\x64\\release\\shallow.x64.exe"];
moves = []

class ChessEngine:
    def __init__(self, enum):
        self.enum = enum
        ename = engines[enum]
        self.p = Popen(ename, stdin = PIPE, stdout = PIPE, shell = True)
        if not self.p:
            exit()
        self.fd = self.p.stdout.fileno()        
        self.fh = msvcrt.get_osfhandle(self.fd)

    def quit(self):
        if self.p:
            self.p.stdin.write('quit\n')
            self.p.stdout.close()
            self.p.stdin.close()
            self.p = None

    def readln(self, timeout):
        t = 0.0
        while t < timeout:
            _, aval, _ = PeekNamedPipe(self.fh, 0)
            if aval > 0:
                return self.p._translate_newlines(self.p.stdout.readline())
            time.sleep(0.01)
            t = t + 0.01
            return None

    def start(self):
        if self.p:
            self.p.stdin.write('uci\n')
            while True:
                line = self.readln(0.05)
                if not line:
                    break;
            self.p.stdin.write('ucinewgame\n')

    def go(self, dt, n):
        if dt < 1.0:
            dt = 1.0
        dtms = int(dt*1000)
        sendstr = 'go wtime %d btime %d movestogo %d\n' % (dtms, dtms, n)
        self.p.stdin.write(sendstr)
        t = 0.0
        depth = 0
        score = 0
        mate = False
        while t < 2*dt:
            line = self.readln(0.1)
            if line:
                tokens = line.split()
                if len(tokens) > 1 and tokens[0] == 'bestmove':
                    return tokens[1], score, depth, mate
                elif len(tokens) > 7:
                    if tokens[5] == 'score':
                        score = int(tokens[7])
                    if tokens[6] == 'mate':
                        mate = True
                    if tokens[1] == 'depth':
                        d = int(tokens[2])
                        if d > depth:
                            depth = d
            t = t + 0.1
            time.sleep(0.1)
        return None, score, depth, mate

    def pos(self):
        strpos = 'position startpos'
        if len(moves) > 0:
            strpos = strpos + ' moves ' + ' '.join(moves)
        strpos = strpos + '\n'
        self.p.stdin.write(strpos)

engs = [None, None]
engs[0] = ChessEngine(0)
engs[1] = ChessEngine(1)
engs[0].start()
engs[1].start()

cur = 1
timebw = [60, 60]
while True:
    engs[cur].pos()
    dt = timebw[cur]
    n = 40 - ((len(moves)/2) % 40)
    t0 = time.clock()
    best, score, depth, mate = engs[cur].go(dt, n)
    t1 = time.clock()
    dt = t1 - t0
    timebw[cur] = timebw[cur] - dt
    if (len(moves) % 80) == 0 and (len(moves) > 0):
        timebw = [60, 60]
    if not best:
        break
    moves.append(best)
    scorestr = score/100.0
    if mate:
        scorestr = 'mate in %d' % (abs(score))
    print len(moves), ':', best, scorestr, depth, timebw[cur]
    cur = (cur + 1) % 2
    if len(moves) > 300:
        break

engs[0].quit()
engs[1].quit()

f = open('result.pgn', 'wt')
f.write('[Event ...]:\n[Site ...]:\n[Date ...]:\n[Round ...]:\n[White ...]:\n[Black ...]:\n[Result ...]:\n[TimeControl ...]:\n')
for i, move in enumerate(moves):
    f.write('%d. %s\n' % (i+1, move))
f.close()

