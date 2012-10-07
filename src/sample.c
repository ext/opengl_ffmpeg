#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "GL/ffgl.h"

int main(int argc, const char* argv[]){
	const unsigned int width = 1920;
	const unsigned int height = width / 16 * 9;
	const unsigned int framerate = 60;
	const float dt = (float)1.0 / framerate;

	if ( ffgl_init(width, height, framerate, "test.mp4") != 0 ){
		fprintf(stderr, "ffgl_init failed: %s\n", ffgl_get_error());
	}

	glClearColor(1,0,1,1);
	glShadeModel( GL_SMOOTH );
	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glEnable( GL_MULTISAMPLE );

	glViewport( 0, 0, width, height);
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluPerspective( 45.0f, (float)width / height, 0.1f, 100.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	for ( int i = 0; i < 1800; i++ ){
		fprintf(stderr, "frame %d   \r", i);
		fflush(stderr);

		static float rtri = 0.0f;
		static float rquad = 0.0f;

		rtri  += 20.0 * dt;
		rquad += 35.0 * dt;

		glClear(GL_COLOR_BUFFER_BIT);

		/* pasta from nehe 05 */
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glLoadIdentity();
		glTranslatef( -1.5f, 0.0f, -6.0f );
		glRotatef( rtri, 0.1f, 1.0f, 0.0f );

		glBegin( GL_TRIANGLES );
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(  0.0f,  1.0f,  0.0f );
		glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( -1.0f, -1.0f,  1.0f );
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(  1.0f, -1.0f,  1.0f );
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(  0.0f,  1.0f,  0.0f );
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(  1.0f, -1.0f,  1.0f );
		glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(  1.0f, -1.0f, -1.0f );
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(  0.0f,  1.0f,  0.0f );
		glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(  1.0f, -1.0f, -1.0f );
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( -1.0f, -1.0f, -1.0f );
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(  0.0f,  1.0f,  0.0f );
		glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( -1.0f, -1.0f, -1.0f );
		glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( -1.0f, -1.0f,  1.0f );
		glEnd( );

		glLoadIdentity( );
		glTranslatef( 1.5f, 0.0f, -6.0f );
		glRotatef( rquad, 1.0f, 0.0f, 0.2f );

		glColor3f( 0.5f, 0.5f, 1.0f);
		glBegin( GL_QUADS );
		glColor3f(   0.0f,  1.0f,  0.0f );
		glVertex3f(  1.0f,  1.0f, -1.0f );
		glVertex3f( -1.0f,  1.0f, -1.0f );
		glVertex3f( -1.0f,  1.0f,  1.0f );
		glVertex3f(  1.0f,  1.0f,  1.0f );
		glColor3f(   1.0f,  0.5f,  0.0f );
		glVertex3f(  1.0f, -1.0f,  1.0f );
		glVertex3f( -1.0f, -1.0f,  1.0f );
		glVertex3f( -1.0f, -1.0f, -1.0f );
		glVertex3f(  1.0f, -1.0f, -1.0f );
		glColor3f(   1.0f,  0.0f,  0.0f );
		glVertex3f(  1.0f,  1.0f,  1.0f );
		glVertex3f( -1.0f,  1.0f,  1.0f );
		glVertex3f( -1.0f, -1.0f,  1.0f );
		glVertex3f(  1.0f, -1.0f,  1.0f );
		glColor3f(   1.0f,  1.0f,  0.0f );
		glVertex3f(  1.0f, -1.0f, -1.0f );
		glVertex3f( -1.0f, -1.0f, -1.0f );
		glVertex3f( -1.0f,  1.0f, -1.0f );
		glVertex3f(  1.0f,  1.0f, -1.0f );
		glColor3f(   0.0f,  0.0f,  1.0f );
		glVertex3f( -1.0f,  1.0f,  1.0f );
		glVertex3f( -1.0f,  1.0f, -1.0f );
		glVertex3f( -1.0f, -1.0f, -1.0f );
		glVertex3f( -1.0f, -1.0f,  1.0f );
		glColor3f(   1.0f,  0.0f,  1.0f );
		glVertex3f(  1.0f,  1.0f, -1.0f );
		glVertex3f(  1.0f,  1.0f,  1.0f );
		glVertex3f(  1.0f, -1.0f,  1.0f );
		glVertex3f(  1.0f, -1.0f, -1.0f );
		glEnd( );

		ffgl_swap();
	}

	ffgl_cleanup();
	return 0;
}
