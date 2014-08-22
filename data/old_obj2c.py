#!/usr/bin/python
from sys import argv, stdout

class Mesh:
	def __init__( self ):
		##self.dim = 0 # Number of dimensions (2 or 3)
		self.verts = []
		self.faces = []
		##self.rawdata = [] # Contains verts and faces as bytes
	def add_vertex( self, vert ):
		self.verts += [vert]
	def add_quad( self, quad ):
		self.faces += [quad]
	def get_bounds( self ):
		min_co=[10000.0,10000.0,10000.0]
		max_co=[-10000.0,-10000.0,-10000.0]
		for vert in self.verts:
			for c in range(3):
				if vert[c] < min_co[c]:
					min_co[c] = vert[c]
				if vert[c] > max_co[c]:
					max_co[c] = vert[c]
		return [min_co, max_co]
	def get_maximum_extents( self ):
		max_co=[]
		bounds=self.get_bounds()
		for n in range(3):
			max_co += [max( abs(bounds[0][n]), abs(bounds[1][n]) )]
		return max_co
	def parse_wavefront_obj( self, file ):
		for line in file.readlines():
			if ( line[0] == 'v' ):
				self.add_vertex( [float(s) for s in line.split()[1:]] )
			elif ( line[0] == 'f' ):
				# All indexes in OBJ begin with 1. How fucking retarted is that...?
				self.add_quad( [(int(s)-1) for s in line.split() if s.isdigit()] )
			# Discard comments, materials, etc..
	def write_c_source( self, out ):
		max_vert_co=self.get_maximum_extents()
		mesh_scale=[1.0 / 127.0 * x for x in max_vert_co]
		#out.write( "/* Scale: " + str( mesh_scale ) + "*/\n" )
		#out.write( "/* Bounding box: " + str( self.get_bounds() ) + "*/\n" )
		#out.write( "/* Max extents: " + str( max_vert_co ) + "*/\n" )
		out.write( "const float MESH_SCALE[] = {%f, %f, %f};\n" % (mesh_scale[0], mesh_scale[1], mesh_scale[2]) )
		out.write( "const int MESH_VERTEX_COUNT = %d;\n" % len(self.verts) )
		out.write( "const int MESH_INDEX_COUNT = %d;\n" % (len(self.faces)*4) )
		dim=3
		if ( max_vert_co[2] < 0.00001 ):
			dim=2
		data_size=len(self.faces)*4+len(self.verts)*dim
		out.write( "const char MESH_DATA[] = { /* %d bytes */\n" % data_size )
		if ( max_vert_co[2] < 0.00001 ):
			# The model seems to be 2D
			# Ignore Z coordinate
			out.write( "\t/* Vertex coordinates (2d) */\n\t" )
			for vert in self.verts:
				c = [ int( vert[n] * 127.0 / max_vert_co[n] ) for n in range(2) ]
				out.write( "%d,%d," % (c[0],c[1]) )
		else:
			# 3D mesh; x,y,z
			out.write( "\t/* Vertex coordinates (3d) */\n\t" )
			for vert in self.verts:
				c = [ int( vert[n] * 127.0 / max_vert_co[n] ) for n in range(3) ]
				out.write( "%d,%d,%d," % (c[0],c[1],c[2]) )
		out.write( "\n\t/* Quad indices */\n\t" )
		for f in self.faces:
			out.write( "%d,%d,%d,%d," % (f[0],f[1],f[2],f[3]) )
		out.write( "\n};\n" )
	def write_c_source_555( self, f ):
		box=self.get_bounds()
		dim=3
		if ( box[0][2] == box[1][2] ):
			dim=2
		size=[box[1][d] - box[0][d] for d in range(dim)]
		max_co=[max(abs(box[0][d]),abs(box[1][d])) for d in range(dim)]
		data_size=len(self.faces)*4+len(self.verts)*2
		f.write( "const unsigned char DATA[] = { /* (%d bytes) */\n" % data_size )
		if ( dim == 3 ):
			f.write( "\t/* (x,y,z) packed in 5/5/5 bits, 2 bytes per vertex */\n\t" )
		else:
			f.write( "\t/* (x,y) as signed bytes, 2 bytes per vertex */\n\t" )
		for v in self.verts:
			dat=0
			if ( dim == 3 ):
				for d in range( 3 ):
					dat |= ( int( v[d] / max_co[d] * 15.0 ) & 0x1f ) << ( d * 5 )
			else:
				dat |= int( v[0] / max_co[0] * 127.0 ) & 0xFF
				dat |= ( int( v[1] / max_co[1] * 127.0 ) & 0xFF ) << 8
			f.write( hex( dat & 0xFF ) + "," + hex( dat >> 8 & 0xFF ) + "," )
		f.write( "\n\t" )
		for q in self.faces:
			for k in range(4):
				f.write( "%d," % q[k] )
		f.write( "\n};\n" )

def obj2c( filename ):
	file=open( filename, "r" )
	mesh=Mesh()
	mesh.parse_wavefront_obj( file )
	file.close()
	mesh.write_c_source( stdout )
	mesh.write_c_source_555( stdout )

if __name__ == "__main__":
	if len( argv ) < 2:
		print "Usage:", argv[0], "[FILENAME]"
		print "  Converts a wavefront OBJ file to C source code and prints the code"
	else:
		for a in argv[1:]:
			print "**************** File: '%s' ****************" % a
			obj2c( a )
