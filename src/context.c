#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

/* for initial testing */
#include <png.h>
#include <zlib.h>

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef Bool (*glXMakeContextCurrentARBProc)(Display*, GLXDrawable, GLXDrawable, GLXContext);
static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
static glXMakeContextCurrentARBProc glXMakeContextCurrentARB = 0;

static int visual_attribs[] = {
	None
};
static int context_attribs[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
	GLX_CONTEXT_MINOR_VERSION_ARB, 0,
	None
};

#define error_len 4096
static char error_buffer[error_len] = {0,};

static Display* dpy = NULL;
static GLXPbuffer pbuf;
static GLXContext ctx;
static unsigned int width;
static unsigned int height;

static void ffgl_set_error(const char* fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	if ( vsnprintf(error_buffer, error_len-1, fmt, ap) < 0 ){ /* -1 so there will always be a null-terminator */
		abort();
	}
	va_end(ap);
}

int ffgl_init(int w, int h){
	width = w;
	height = h;

	/* open display */
	if ( ! (dpy = XOpenDisplay(0)) ){
		ffgl_set_error("failed to open display");
		return 1;
	}

	int fbcount = 0;
	GLXFBConfig* fbc = NULL;

	/* get framebuffer configs, any is usable (might want to add proper attribs) */
	if ( !(fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attribs, &fbcount) ) ){
		ffgl_set_error("failed to get FBConfig");
		return 1;
	}

	/* get the required extensions */
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB");
	glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB( (const GLubyte *) "glXMakeContextCurrent");
	if ( !(glXCreateContextAttribsARB && glXMakeContextCurrentARB) ){
		ffgl_set_error("missing support for GLX_ARB_create_context");
		XFree(fbc);
		return 1;
	}

	/* create a context using glXCreateContextAttribsARB */
	if ( !( ctx = glXCreateContextAttribsARB(dpy, fbc[0], 0, True, context_attribs)) ){
		ffgl_set_error("failed to create opengl context");
		XFree(fbc);
		return 1;
	}

	/* create temporary pbuffer */
	int pbuffer_attribs[] = {
		GLX_PBUFFER_WIDTH, width,
		GLX_PBUFFER_HEIGHT, height,
		None
	};
	pbuf = glXCreatePbuffer(dpy, fbc[0], pbuffer_attribs);

	XFree(fbc);
	XSync(dpy, False);

	/* try to make it the current context */
	if ( !glXMakeContextCurrent(dpy, pbuf, pbuf, ctx) ){
		/* some drivers does not support context without default framebuffer, so fallback on
		 * using the default window.
		 */
		if ( !glXMakeContextCurrent(dpy, DefaultRootWindow(dpy), DefaultRootWindow(dpy), ctx) ){
			ffgl_set_error("failed to make current");
			return 1;
		}
	}

	return 0;
}

int ffgl_cleanup(){
	return 0;
}

int ffgl_poll(){
	return 0;
}

int ffgl_swap(){
	glXSwapBuffers(dpy, pbuf);

	unsigned char buffer[width*height*4];
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

	FILE *fp = fopen("test.png", "wb");
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr, "[write_png_file] Error during init_io\n");
		abort();
	}

	png_init_io(png_ptr, fp);

	/* write header */
	png_set_IHDR(png_ptr, info_ptr, width, height,
	             8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);
	//png_write_image(png_ptr, buffer);
	for ( unsigned int y = 0; y < height; ++y ){
		unsigned char* row = &buffer[y * width * 4];
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, NULL);

	return 0;
}

const char* ffgl_get_error(){
	return error_buffer;
}
