import os, os.path, string

f = open(r'debut.tbl')
lines = f.readlines()
f.close()

f = open('..\engine\debut.cpp', 'wt')
n = len(lines)
m = max(map(lambda x : len(x.split()), lines))
f.write("#include \"Helpers.h\"\n\nconst char * DebutsTable::s_table_[%d][%d] = {\n" % (n, m))
for i, line in enumerate(lines):
	if i > 0:
		f.write(",\n")
	f.write("  { ")
	for j, move in enumerate(line.split()):
		if j > 0:
			f.write(', ');
		f.write('"%s"' % (move))
	f.write(" }");
f.write("\n};\n")
f.close()