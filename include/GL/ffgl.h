#ifndef FFGL_H
#define FFGL_H

int ffgl_init(int width, int height);
int ffgl_cleanup();
int ffgl_poll();
int ffgl_swap();

const char* ffgl_get_error();

#endif /* FFGL_H */
