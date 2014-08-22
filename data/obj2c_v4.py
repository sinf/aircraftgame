#!/usr/bin/python
from sys import argv, stdout
from os.path import basename
from struct import pack, unpack

def clamp( x, low, high ):
	if x < low:
		x = low
	if x > high:
		x = high
	return x

class BitStream:
	def __init__( self ):
		self.bits=[]
	def add_int( self, x, bits ):
		for n in range( bits ):
			bit = ( x >> n ) & 0x1
			self.bits += [bit]
	def pad( self, pad_size ):
		n = len( self.bits ) % pad_size
		if n != 0:
			self.add_int( 0, pad_size-n )
	def get_byte_string( self ):
		s=""
		byte=0
		shift=7
		self.pad( 8 )
		for bit in self.bits:
			byte |= ( bit << shift )
			shift -= 1
			if shift < 0:
				s += pack( "B", byte )
				byte=0
				shift=7
		return s
	def get_formatted_byte_string( self ):
		s=""
		for byte in self.get_byte_string():
			s+=hex( unpack("B",byte)[0] ) + ","
		s=s[:-1] # Remove the last comma
		return s

class Mesh:
	def __init__( self, obj_file=None ):
		self.dim = 0 # Number of dimensions (2 or 3)
		# Multiply normalized vertex coords with this scale to get the actual coordinates:
		self.scale = [1.0,1.0,1.0]
		self.verts = []
		self.faces = []
		if ( obj_file is not None ):
			self.parse_wavefront_obj( obj_file )
	def add_vertex( self, vert ):
		self.verts += [vert]
	def add_quad( self, quad ):
		self.faces += [quad]
	def get_bounds( self ):
		""" Returns the bounding box """
		min_co=[10000.0,10000.0,10000.0]
		max_co=[-10000.0,-10000.0,-10000.0]
		for vert in self.verts:
			for c in range(3):
				if vert[c] < min_co[c]:
					min_co[c] = vert[c]
				if vert[c] > max_co[c]:
					max_co[c] = vert[c]
		return [[min_co[n],max_co[n]] for n in range(3)]
		#return [min_co, max_co]
	def parse_wavefront_obj( self, file ):
		""" Loads the mesh from a OBJ file """
		self.verts=[]
		self.faces=[]
		do_close=False
		if ( type(file) == str ):
			# The file argument was a filename instead of a file
			# Need to open/close the file
			file=open( file, "r" )
			do_close=True
		for line in file.readlines():
			if ( line[0] == 'v' ):
				self.add_vertex( [float(s) for s in line.split()[1:]] )
			elif ( line[0] == 'f' ):
				# All indexes in OBJ begin with 1. How fucking retarted is that...?
				self.add_quad( [(int(s)-1) for s in line.split() if s.isdigit()] )
			# Discard comments, materials, etc..
		if ( do_close ):
			file.close()
		# Compute mesh scale
		bounds=self.get_bounds()
		self.scale=[max(abs(co[0]), abs(co[1])) for co in bounds]
		# Normalize the vertices to range [-1,1]
		for v in self.verts:
			for n in range(3):
				scale=self.scale[n]
				if ( scale < 0.00001 ):
					v[n] = 0
				else:
					v[n] /= scale
		# Check if the mesh is 2D or not
		if self.scale[2] < 0.00001:
			self.dim = 2
		else:
			self.dim = 3
	def get_vertex_array_string( self, prefix="" ):
		s="const signed char %s_verts[%d*%d] = {" % ( prefix, len( self.verts ), self.dim )
		for vert in self.verts:
			for b in [ int( vert[n] * 127.5 - 1.0 ) for n in range( self.dim ) ]:
				s += "%d," % clamp( b, -128, 127 )
		s = s[:-1] + "};"
		return s
	def get_index_array_string( self, prefix="" ):
		s="const unsigned char %s_indices[%d] = {" % ( prefix, len(self.faces) * 4 )
		for quad in self.faces:
			for b in quad:
				s += "%d," % b
		s = s[:-1] + "};"
		return s
	def dump_text( self, f, prefix ):
		f.write( "/* scale: %g, %g, %g */\n" %( self.scale[0], self.scale[1], self.scale[2] ))
		f.write( self.get_vertex_array_string( prefix ) + "\n" )
		f.write( self.get_index_array_string( prefix ) + "\n" )
	def write_obj( self, f ):
		for v in self.verts:
			f.write( "v %g %g %g\n" % (v[0],v[1],v[2]) )
		for q in self.faces:
			f.write( "f %d %d %d %d\n" % (q[0]+1,q[1]+1,q[2]+1,q[3]+1) )
	def get_vertex_array_string_5bit_s( self ):
		bs=BitStream()
		for vert in self.verts:
			for n in range( self.dim ):
				x=int( vert[n] * 15.5 - 1.0 )
				x=clamp( x, -16, 15 )
				bs.add_int( x, 5 )
		return bs.get_formatted_byte_string()
	def get_index_array_string_5bit_u( self ):
		bs=BitStream()
		for face in self.faces:
			for index in face:
				x=( index & 0b11111 )
				bs.add_int( x, 5 )
		return bs.get_formatted_byte_string()
	def dump_text_5bit( self, f, prefix ):
		f.write( "/* %s\n" )
		f.write( "   scale: %g, %g, %g\n" %( self.scale[0], self.scale[1], self.scale[2] ))
		f.write( "   verts=%d (%d-D); indices=%d */\n" %( len(self.verts), self.dim, len(self.faces)*4 ))
		f.write( "/* %s 5-bit verts */ %s\n" % (prefix, self.get_vertex_array_string_5bit_s()) )
		f.write( "/* %s 5-bit indices */ %s\n" % (prefix, self.get_index_array_string_5bit_u()) )

def test_dump( me, filename ):
	print "Test dump ->", filename
	file=open(filename,"w")
	me.write_obj( file )
	file.close()

def main():
	if len(argv) < 2:
		print "Usage:", argv[0], "[FILENAME(s)]"
		print "  Formats data from wavefront OBJ file(s) to C-like arrays"
	else:
		for filename in argv[1:]:
			print "\n/* filename:", filename, "*/"
			me = Mesh()
			me.parse_wavefront_obj( filename )
			name=basename( filename )
			name=name.rsplit('.',1)[0]
			me.dump_text( stdout, name )
			#me.dump_text_5bit( stdout, name )
			#test_dump( me, filename+".test_dump.obj" )

if __name__ == "__main__":
	main()
