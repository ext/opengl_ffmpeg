all: sample

sample: src/context.o src/sample.o
	$(CC) $(LDFLAGS) $^ -lpng -lGLU -lavformat -lswscale -g -lX11 -lGL -o $@

%.o: %.c
	 $(CC) -Wall -Iinclude -std=c99 $(CFLAGS) -c $< -o $@

clean:
	rm -rf sample src/*.o
