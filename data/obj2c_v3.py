#!/usr/bin/python
from sys import argv, stdout
from random import randint, seed
from copy import deepcopy
import struct

from OpenGL.GLUT import *
from OpenGL.GLU import *
from OpenGL.GL import *

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
	def pack_data( self, use_5bit_verts=False ):
		""" Updates the byte array """
		data = ""
		# Write vertex coordinates:
		if use_5bit_verts:
			# 2D and 3D: 2 bytes per vertex (5/5/5 + 1 unused bit)
			for vert in self.verts:
				c=[ int( co * 15.0 ) & 0b11111 for co in vert]
				u=0
				for n in range(3):
					u |= c[n] << ( 5 * n )
				data += struct.pack( "H", u )
		else:
			if self.dim == 2:
				# 2D: 2 bytes per vertex
				for vert in self.verts:
					c = [ int( vert[n] * 127.0 ) for n in range(2) ]
					data += struct.pack( "bb", c[0], c[1] )
			else:
				# 3D: 3 bytes per vertex
				for vert in self.verts:
					c = [ int( vert[n] * 127.0 ) for n in range(3) ]
					data += struct.pack( "bbb", c[0], c[1], c[2] )
		# Sort faces so that the last faces contain bigger indices than the first faces
		# (might make the data more compressible)
		self.faces = sorted( self.faces, key=lambda q: q[0]+q[1]+q[2]+q[3] )
		# Write face indices:
		for f in self.faces:
			data += struct.pack( "bbbb", f[0], f[1], f[2], f[3] )
		return data
	def dump_text( self, f ):
		""" Writes the mesh data in human readable ASCII format """
		f.write( "Scale: "+str(self.scale)+"\n" )
		f.write( "%d-D vertex coords: %d\n" % (self.dim, len(self.verts)) )
		f.write( "Quad indices: %d\n" % (len(self.faces)*4) )
		# Dump 8-bit data
		data=self.pack_data()
		f.write( "char DATA[] = { /* Size=%d. Verts=8bit; Indices=8bit */\n\t" % len(data) )
		for c in data:
			f.write( "%d," % struct.unpack( 'b',c ) )
		f.write( "\n};\n" )
		if self.dim == 3:
			# Dump 5/5/5 data
			data555=self.pack_data( use_5bit_verts=True )
			f.write( "char DATA555[] = { /* Size=%d. Verts=5bit. Indices=8bit */\n\t" % len(data555) )
			for c in data555:
				f.write( "%d," % struct.unpack( 'b',c ) )
			f.write( "\n};\n" )
	def render( self ):
		glEnableClientState( GL_VERTEX_ARRAY )
		glVertexPointer( 3, GL_FLOAT, 0, self.verts )
		# Push
		glPushMatrix()
		glScalef( self.scale[0], self.scale[1], self.scale[2] )
		# Green quads
		glColor3f( 0.0, 1.0, 0.0 )
		glDrawElements( GL_QUADS, 4*len(self.faces), GL_UNSIGNED_INT, self.faces )
		# Red vertices
		glColor3f( 1.0, 0.0, 0.0 )
		glDrawArrays( GL_POINTS, 0, len(self.verts) )
		# Pop
		glPopMatrix()
		glDisableClientState( GL_VERTEX_ARRAY )

the_mesh=Mesh()
rotation=0.0

def idle_func( junk ):
	global rotation
	ticks=30
	dt=1.0/ticks
	rotation+=dt*30
	glutTimerFunc( 1000/ticks, idle_func, 0 )
	glutPostRedisplay()

def set_projection():
	glMatrixMode( GL_PROJECTION )
	gluPerspective( 40.0, 1.0, 1.0, 40.0 )
	glMatrixMode( GL_MODELVIEW )
	gluLookAt(0,0,10, 0,0,0, 0,1,0)
	glPushMatrix()

def initGL():
	glClearColor( 0.0, 0.0, 0.0, 1.0 )
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE )
	glLineWidth( 2.0 )
	glPointSize( 5.0 )

def show_gui():
	win_w=512
	win_h=512
	glutInit( argv )
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH )
	glutInitWindowSize( win_w, win_h )
	glutCreateWindow( "obj2c" )
	glutDisplayFunc( display )
	glutTimerFunc( 1, idle_func, 0 )
	glViewport( 0, 0, win_w, win_h )
	set_projection()
	initGL()
	glutMainLoop()

def display():
	global rotation
	global the_mesh
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT )
	glPushMatrix()
	glRotatef( rotation, 0.0, 1.0, 0.0 )
	glColor3f( 1.0, 0.0, 0.0 )
	the_mesh.render()
	glPopMatrix()
	glutSwapBuffers()

def mouse_event( button, state, x, y ):
	return

def main():
	if len(argv) < 2:
		print "Usage:", argv[0], "[FILENAME(s)]"
		print "  Converts a wavefront OBJ file to C source code and prints the code"
		print "	If only 1 filename is given then a 3D preview shows up"
	else:
		global the_mesh
		if len(argv) == 2:
			filename=argv[1]
			print "Filename:", filename
			the_mesh.parse_wavefront_obj( filename )
			the_mesh.dump_text( stdout )
			show_gui()
		else:
			for filename in argv[1:]:
				print "\nFilename:", filename
				the_mesh.parse_wavefront_obj( filename )
				the_mesh.dump_text( stdout )

if __name__ == "__main__":
	main()

"""
if __name__ == "__main__":
	if len( argv ) < 2:
		print "Usage:", argv[0], "[FILENAME]"
		print "  Converts a wavefront OBJ file to C source code and prints the code"
	else:
		for a in argv[1:]:
			print "**************** File: '%s' ****************" % a
			mesh=Mesh( a )
			mesh.dump_text( stdout )
"""
