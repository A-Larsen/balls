LIBS = -lSDL2 -lSDL2_ttf -lm

PROG = balls

build: $(PROG).c
	gcc -g -o $(PROG) $(PROG).c $(LIBS)

clean:
	rm -rf $(PROG)

.PHONY: clean
