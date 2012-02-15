#ifndef FFGL_H
#define FFGL_H

int ffgl_init(unsigned int width, unsigned int height, unsigned int framerate, const char* filename);
int ffgl_cleanup();
int ffgl_poll();
int ffgl_swap();

const char* ffgl_get_error();

#endif /* FFGL_H */
