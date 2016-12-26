#!/usr/bin/python

from subprocess import Popen, PIPE
import subprocess, time, errno, os, os.path
from random import shuffle, randint

eng_paths = ['d:/projects/gitproj/shallow/build_x64_vc2013/release', 'C:\Program Files (x86)\Arena\Engines\shallow']
eng_names = ['shallow.exe', 'shallow_no_pw_penl.exe']#'stockfish_8_x64_popcnt.exe']
engines = [os.path.join(eng_paths[1], eng_names[0]), os.path.join(eng_paths[1], eng_names[1])]

givenMoves_ = 40
givenTime_ = 240
movesLimit_ = 500
numberOfGames_ = 40
resultFname_ = 'result.pgn'
rand_seq = []

if subprocess.mswindows:
  import msvcrt
  from win32pipe import PeekNamedPipe

  def get_pipe_handle(file_handle):
    return msvcrt.get_osfhandle(file_handle)

  def peek_input(pipe_handle, p):
    _, aval, _ = PeekNamedPipe(pipe_handle, 0)
    if aval > 0:
      return p._translate_newlines(p.stdout.readline())
    return None

else:
  import fcntl, select

  def get_pipe_handle(file_handle):
    return file_handle

  def peek_input(pipe_handle, p):
    conn = pipe_handle# r[0]
    flags = fcntl.fcntl(conn, fcntl.F_GETFL)
    fcntl.fcntl(conn, fcntl.F_SETFL, flags | os.O_NONBLOCK)
    try:
      data = p._translate_newlines(p.stdout.readline())
    except IOError as e:
      data = None
    fcntl.fcntl(conn, fcntl.F_SETFL, flags)
    return data

moves = []
def loadMoves(fname, n):
  global moves, rand_seq
  with open(fname, 'rt') as f:
    lines = f.readlines()
  if len(rand_seq) == 0:
    rand_seq = range(0, len(lines)-1)
    shuffle(rand_seq)
  moves = lines[rand_seq[n % len(lines)]].split()[:12]

class ChessEngine:
  def __init__(self, enum, ename = ''):
    self.enum = enum
    if ename == '':
      self.ename = engines[enum]
    else:
      self.ename = ename
    self.p = Popen(self.ename, stdin = PIPE, stdout = PIPE, shell = True)
    if not self.p:
      print 'could not start process'
      exit()
    self.fd = self.p.stdout.fileno()
    self.fh = get_pipe_handle(self.fd)
    self.engtitle = os.path.basename(self.ename)

  def quit(self):
    if self.p:
      self.p.stdin.write('quit\n')
      time.sleep(0.5)
      self.p.stdout.close()
      self.p.stdin.close()
      self.p = None

  def readln(self, timeout):
    t = 0.0
    while t < timeout:
      data = peek_input(self.fh, self.p)
      if data:
        return data
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
    bestmove = None
    while t < 2*dt and not bestmove:
      line = self.readln(0.1)
      if line:
        tokens = line.split()
#       print tokens
        for i, tok in enumerate(tokens):
          if i >= len(tokens)-1:
            break
          if tok == 'bestmove':
            bestmove = tokens[i+1]
          if tok == 'score':
            score = int(tokens[i+2])
          if tok == 'mate':
            mate = True
            score = int(tokens[i+1])
          if tok == 'depth':
            d = int(tokens[i+1])
            if d > depth:
              depth = d
      t = t + 0.1
      time.sleep(0.1)
    return bestmove, score, depth, mate

  def pos(self):
    strpos = 'position startpos'
    if len(moves) > 0:
      strpos = strpos + ' moves ' + ' '.join(moves)
    strpos = strpos + '\n'
    self.p.stdin.write(strpos)
    
  def save(self, fname):
    strsave = 'saveboard ' + fname + '\n'
    self.p.stdin.write(strsave)
    
results = [0, 0]
whiteNo = 1
engs = [None, None]

def resultDraw():
  global results
  for i in [0, 1]:
    results[i] = results[i] + 0.5

def printResult():
  print 'Result: ', engs[0].engtitle, '=', results[0], ', ', engs[1].engtitle, '=', results[1]

for num in xrange(0, numberOfGames_):
  loadMoves('../tables/Kaissa.bk', num)
  print 'Game:', num

  whiteNo = (whiteNo + 1) % 2
  engs[0] = ChessEngine(0)
  engs[1] = ChessEngine(1)
  print 'White:', engs[whiteNo].engtitle
  print 'Black:', engs[(whiteNo+1)%2].engtitle
  engs[0].start()
  engs[1].start()

  for i, m in enumerate(moves):
    print i, ':', m

  cur = whiteNo
  timebw = [givenTime_, givenTime_]
  while True:
    engs[cur].pos()
    dt = timebw[cur]
    if dt < 1:
      dt = 1
    n = givenMoves_ - ((len(moves)/2) % givenMoves_)
    t0 = time.time()
    best, score, depth, mate = engs[cur].go(dt, n)
    t1 = time.time()
    dt = t1 - t0
    timebw[cur] = timebw[cur] - dt
    if timebw[cur] < 1:
      timebw[cur] = 1
    if (len(moves) % (2*givenMoves_)) == 0 and (len(moves) > 0):
      timebw = [givenTime_, givenTime_]
    if not best or best == 'draw':
      resultDraw()
      break
    moves.append(best)
    scorestr = 'score %4.1f' % (score/100.0)
    mate_in = 100000;
    if mate:
      mate_in = abs(score)
      scorestr = 'mate in %d' % (mate_in)
    if mate_in == 0:
      results[cur] = results[cur] + 1
    print len(moves), ': bestmove', best, ', ', scorestr, ', depth', depth, ', time left', int(timebw[cur]), ', engine ', engs[cur].engtitle
    cur = (cur + 1) % 2
    if len(moves) > movesLimit_ or mate_in == 0:
      break

  printResult()
  engs[0].pos()
  with open(resultFname_, 'at') as f:
    f.write('\n[White \"%s\"]:\n[Black \"%s\"]\n' % (engs[whiteNo].engtitle, engs[(whiteNo+1)%2].engtitle))
  engs[0].save(resultFname_)
  engs[0].quit()
  engs[1].quit()

with open(resultFname_, 'at') as f:
  f.write('\n{ Result: ' + engs[0].engtitle + '=' + str(results[0]) + ', ' + engs[1].engtitle + '=' + str(results[1]) + ' }\n')
