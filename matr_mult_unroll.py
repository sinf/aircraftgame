
epsilon=0.0001

def mult( a, b ):
	if type(a) is str:
		if type(b) is not str:
			if abs(b) < epsilon: return 0.0; # multiply by zero
			if abs(b-1.0) < epsilon: return a; # multiply by one
		return "%s*%s" % (a,b) # 2 variables. Can't precalculate
	if type(b) is str:
		if type(a) is not str:
			if abs(a) < epsilon: return 0.0;
			if abs(a-1.0) < epsilon: return b;
		return "%s*%s" % (a,b)
	return a*b;

def add( a, b ):
	if type(a) is str:
		if ( type(b) is not str ) and ( abs(b) < epsilon ): return a; # add zero
		return "(%s+%s)" % (a,b)
	if type(b) is str:
		if ( type(a) is not str ) and ( abs(a) < epsilon ): return b;
		return "(%s+%s)" % (a,b)
	return a+b;

def transpose( a ):
	b0=[a[0],a[4],a[8],a[12]]
	b1=[a[1],a[5],a[9],a[13]]
	b2=[a[2],a[6],a[10],a[14]]
	b3=[a[3],a[7],a[11],a[15]]
	return b0+b1+b2+b3

def mat_mult( a, b ):
	c=[]
	for j in range(4):
		for i in range(4):
			d=0
			for k in range(4):
				d = add( d, mult( a[j+4*k], b[4*i+k] ) )
			c += [d]
	return c

def print_mat( m ):
	for r in range(4):
		print("%s %s %s %s" % tuple([str(m[4*i+r]) for i in range(4)]))

def print_mat_out( m ):
	for n in range(16):
		print("out[%d] = %s;" % (n,str(m[n])))

m_translate = [1,0,0,0] + [0,1,0,0] + [0,0,1,0] + ["tx","ty","tz",1]
m_rx = [1,0,0,0] + [0,"c","-s",0] + [0,"s","c",0] + [0,0,0,1]
m_ry = ["c",0,"s",0] + [0,1,0,0] + ["-s",0,"c",0] + [0,0,0,1]
m_rz = ["c","-s",0,0] + ["s","c",0,0] + [0,0,1,0] + [0,0,0,1]
m_scale = ["x", 0, 0, 0] + [0, "y", 0, 0] + [0, 0, "z", 0] + [0,0,0,1]

if __name__ == "__main__":
	for mm in [(m_rx,"case 0:"),(m_ry,"case 1:"),(m_rz,"case 2:")]:
		a=[("in[%d]" % x) for x in range(16)]
		b=mm[0]
		print(mm[1])
		#print_mat( a )
		#print_mat( b )
		print_mat_out( transpose( mat_mult( a, b ) ) )
		print("break;")
	print("Scaling:")
	a=[("in[%d]" % x) for x in range(16)]
	b=m_scale
	print_mat_out( transpose( mat_mult( a, b ) ) )
