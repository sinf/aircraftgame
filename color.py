#!/usr/bin/python3

def conv(x):
	if x[0] == '#':
		x=x[1:]
	elif x[:2] == '0x':
		x=x[2:]
	m=[]
	for i in range(len(x)//2):
		m += [int(x[2*i:2*i+2], 16) / 255.0]
	return tuple(m)

from sys import argv
for x in argv[1:]:
	print(', '.join('%.3f' % y for y in conv(x)))

