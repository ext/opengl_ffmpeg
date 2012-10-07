#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

/* for initial testing */
#include <png.h>
#include <zlib.h>

#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
//#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef Bool (*glXMakeContextCurrentARBProc)(Display*, GLXDrawable, GLXDrawable, GLXContext);
static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
static glXMakeContextCurrentARBProc glXMakeContextCurrentARB = 0;

static int visual_attribs[] = {
	GLX_X_RENDERABLE    , True,
	GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
	GLX_RENDER_TYPE     , GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
	GLX_RED_SIZE        , 8,
	GLX_GREEN_SIZE      , 8,
	GLX_BLUE_SIZE       , 8,
	GLX_ALPHA_SIZE      , 8,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	GLX_DOUBLEBUFFER    , True,
	GLX_SAMPLE_BUFFERS  , 1,
	GLX_SAMPLES         , 4,
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
static AVOutputFormat* fmt = NULL;
static AVFormatContext* oc = NULL;
static AVStream* st = NULL;
static AVCodec* codec = NULL;
static AVCodecContext* c = NULL;
static uint8_t *video_outbuf;
static int video_outbuf_size;
static AVFrame *picture, *tmp_picture;
struct SwsContext* converter = NULL;
static unsigned char* buffer = NULL;

static void ffgl_set_error(const char* fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	if ( vsnprintf(error_buffer, error_len-1, fmt, ap) < 0 ){ /* -1 so there will always be a null-terminator */
		abort();
	}
	va_end(ap);
}

static AVFrame *alloc_picture(int pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;

	const size_t size = avpicture_get_size(pix_fmt, width, height);
	picture_buf = av_malloc(size);
	if (!picture_buf) {
		av_free(picture);
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf,
	               pix_fmt, width, height);
	return picture;
}

int ffgl_init(unsigned int w, unsigned int h, unsigned int framerate, const char* filename){
	width = w;
	height = h;

	buffer = malloc(width*height*4);

	/* initialize libavcodec, and register all codecs and formats */
	av_register_all();

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

	fmt = av_guess_format(NULL, filename, NULL);
	if (!fmt) {
		printf("Could not deduce output format from file extension: using MPEG.\n");
		fmt = av_guess_format("mpeg", NULL, NULL);
	}
	if (!fmt) {
		fprintf(stderr, "Could not find suitable output format\n");
		exit(1);
	}

	/* allocate the output media context */
	oc = avformat_alloc_context();
	if (!oc) {
		fprintf(stderr, "Memory error\n");
		exit(1);
	}
	oc->oformat = fmt;
	snprintf(oc->filename, sizeof(oc->filename), "%s", filename);

	codec = avcodec_find_encoder(CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	st = avformat_new_stream(oc, codec);
	if (!st) {
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}
	st->id = 1;

	c = st->codec;
	c->codec_id = CODEC_ID_H264;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	//c->bit_rate = 400000;
	c->width = width;
	c->height = height;
	c->time_base.den = framerate;
	c->time_base.num = 1;
	//c->gop_size = 12; /* emit one intra frame every twelve frames at most */
	//c->gop_size = 25;
	c->pix_fmt = PIX_FMT_YUV420P;
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	/* set the output parameters (must be done even if no
	   parameters). */
	if (av_set_parameters(oc, NULL) < 0) {
		fprintf(stderr, "Invalid output format parameters\n");
		exit(1);
	}

	av_dump_format(oc, 0, filename, 1);

	/* open the codec */
	AVDictionary* conf = NULL;
	av_dict_set(&conf, "crf", "0", 0);
	av_dict_set(&conf, "preset", "veryslow", 0);
	if (avcodec_open2(c, codec, &conf) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	video_outbuf_size = 2000000;
	video_outbuf = av_malloc(video_outbuf_size);

	/* allocate the encoded raw picture */
	picture = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!picture) {
		fprintf(stderr, "Could not allocate picture\n");
		exit(1);
	}

	tmp_picture = alloc_picture(PIX_FMT_BGRA, width, height);
	if (!tmp_picture) {
		fprintf(stderr, "Could not allocate temporary picture\n");
		exit(1);
	}

	/* open the output file */
	if (avio_open(&oc->pb, filename, URL_WRONLY) < 0) {
		fprintf(stderr, "Could not open '%s'\n", filename);
		exit(1);
	}

	/* write the stream header, if any */
	avformat_write_header(oc, NULL);

	converter = sws_getContext(
		width, height, PIX_FMT_BGRA,
		width, height, c->pix_fmt,
		SWS_FAST_BILINEAR, NULL, NULL, NULL
		);

	return 0;
}

/**
 * Encode and write frame to stream.
 * @param frame If non-null the YUV402P encoded frame is written, if null the
 * buffer is flushed.
 */
static int ffgl_write_frame(const struct AVFrame* frame){
	static int64_t frame_counter = 0;
	picture->pts = frame_counter++;

	const size_t out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, frame);

	/* If out_size is zero the data was buffered */
	if ( out_size == 0 ){
		return 0;
	}

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.pts = pkt.dts = AV_NOPTS_VALUE;

	if (c->coded_frame->pts != AV_NOPTS_VALUE){
		pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
	}

	if(c->coded_frame->key_frame)
		pkt.flags |= AV_PKT_FLAG_KEY;

	pkt.stream_index = st->index;
	pkt.data = video_outbuf;
	pkt.size = out_size;

	return av_write_frame(oc, &pkt);
}

int ffgl_cleanup(){
	ffgl_write_frame(NULL); /* flush buffer */
	av_write_trailer(oc);
	return 0;
}

int ffgl_poll(){
	return 0;
}

int ffgl_swap(){
	glXSwapBuffers(dpy, pbuf);

	/** @todo ensure a fixed frame-rate. */

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

	/* OpenGL stores pixels with flipped y-axis, so in the same step as converting
	 * to YUV it also flips the pixels by reading from the end of the buffer and
	 * moving towards the start. */
	const uint8_t* data = buffer + width*height*4;
	const uint8_t* tmp[4] = { data, NULL, NULL, NULL };
	int stride[4] = { -width*4, 0, 0, 0 };

	sws_scale(converter, tmp, stride,
	          0, height, picture->data, picture->linesize);

	return ffgl_write_frame(picture);
}

const char* ffgl_get_error(){
	return error_buffer;
}
