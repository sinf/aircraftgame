#ifndef _MY_GL_HEADER_GUARD
#define _MY_GL_HEADER_GUARD

#include <GL/gl.h>

#define RGBA_32( r, g, b, a ) ((r)|((g)<<8)|((b)<<16)|((a)<<24))
#define RGB_32( r, g, b ) RGBA_32( r, g, b, 0xFF )

#define VT_FLOAT GL_FLOAT
#define VT_SHORT GL_SHORT
#define VT_INT GL_INT

#define set_color glColor4ub
#define set_color_p glColor4ubv
#define set_vertex_pointer( p, dimensions, type ) glVertexPointer( dimensions, type, 0, p )

#define draw_quads( count ) glDrawArrays( GL_QUADS, 0, (count)<<2 )
#define draw_quads_indexed( index_p, index_size, count ) glDrawElements( GL_QUADS, (count)<<2, \
	(((index_size) == 4) ? GL_UNSIGNED_INT : ( ((index_size) == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE )), \
	index_p )

#define rotate glRotatef
#define scale2(x,y) glScalef(x,y,1)
#define scale3 glScalef
#define translate2(x,y) glTranslatef(x,y,0)
#define translate3 glTranslatef

#endif
