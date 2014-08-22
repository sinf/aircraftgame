#!/usr/bin/python
from sys import argv, stdout
from math import log
from random import randint, seed
from copy import deepcopy
import struct

class Mesh:
	def __init__( self, obj_file=None ):
		self.dim = 0 # Number of dimensions (2 or 3)
		self.verts = []
		self.faces = []
		self.data = None # Contains verts and faces as bytes
		if ( obj_file is not None ):
			self.parse_wavefront_obj( obj_file )
	def add_vertex( self, vert ):
		self.verts += [vert]
	def add_quad( self, quad ):
		self.faces += [quad]
	def parse_wavefront_obj( self, file ):
		""" Loads the mesh from a OBJ file """
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
		return [min_co, max_co]
	def get_maximum_extents( self ):
		""" Returns largest absolute vertex coordinates """
		max_co=[]
		bounds=self.get_bounds()
		for n in range(3):
			max_co += [max( abs(bounds[0][n]), abs(bounds[1][n]) )]
		return max_co
	def pack_data( self ):
		""" Updates the byte array """
		max_vert_co = self.get_maximum_extents()
		mesh_scale = [1.0 / 127.0 * x for x in max_vert_co]
		self.data = ""
		# Write vertex coordinates:
		if ( max_vert_co[2] < 0.00001 ):
			# 2D
			self.dim = 2
			for vert in self.verts:
				c = [ int( vert[n] * 127.0 / max_vert_co[n] ) for n in range(2) ]
				self.data += struct.pack( "bb", c[0], c[1] )
		else:
			# 3D
			self.dim = 3
			for vert in self.verts:
				c = [ int( vert[n] * 127.0 / max_vert_co[n] ) for n in range(3) ]
				self.data += struct.pack( "bbb", c[0], c[1], c[2] )
		# Sort faces so that the last faces contain bigger indices than the first faces
		# (might make the data more compressible)
		self.faces = sorted( self.faces, key=lambda q: q[0]+q[1]+q[2]+q[3] )
		# Write face indices:
		for f in self.faces:
			self.data += struct.pack( "bbbb", f[0], f[1], f[2], f[3] )
	def dump_text( self, f ):
		""" Writes the mesh data in human readable ASCII format """
		self.pack_data()
		max_vert_co=self.get_maximum_extents()
		mesh_scale=[1.0 / 127.0 * x for x in max_vert_co]
		#f.write( "Entropy: %f\n" % get_entropy(self.data) )
		f.write( "Scale: ( %f, %f, %f )\n" % (mesh_scale[0], mesh_scale[1], mesh_scale[2]) )
		f.write( "%d-D vertex coords: %d\n" % (self.dim, len(self.verts)) )
		f.write( "Quad indices: %d\n" % (len(self.faces)*4) )
		f.write( "char DATA[] = { /* size=%d */\n\t" % len(self.data) )
		for c in self.data:
			f.write( "%d," % struct.unpack( 'b',c ) )
		f.write( "\n};\n" )

if __name__ == "__main__":
	if len( argv ) < 2:
		print "Usage:", argv[0], "[FILENAME]"
		print "  Converts a wavefront OBJ file to C source code and prints the code"
	else:
		for a in argv[1:]:
			print "**************** File: '%s' ****************" % a
			mesh=Mesh( a )
			mesh.dump_text( stdout )
