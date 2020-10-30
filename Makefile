.PHONY: all clean format benchmark install test profile documentation

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

cc := gcc
incdirs := -Iinc
ccargscommon := -DVERSION=\"$(GIT_VERSION)\" -std=c99 -Wall -Wextra -pedantic ${incdirs}
ccargsdebug := ${ccargscommon} -Werror -march=native -O3 -g -c
ccargscentos := ${ccargscommon} -march=native -O3 -s
linkargsdebug := -g

modules := main pqueue parg rnd selist stats task ts job json jobgen jobq pqueue eventloop dump
src := $(addsuffix .c, $(addprefix src/, ${modules}))
obj := $(addsuffix .o, ${modules})


all: thready


%.o: src/%.c
	${cc} ${ccargsdebug} $<


threadydebug: ${obj}
	${cc} -o $@ $^ ${linkargsdebug} -lm


thready: ${src}
	${cc} ${ccargscentos} -o $@ $^ -lm


clean:
	-rm *.o
	-rm thready threadydebug
	-rm callgrind.out*
	-rm -r build/doc/*


format:
	find . \( -name \*.c -or -name \*.h \) | xargs -n12 -P4 clang-format -style="{BasedOnStyle: Chromium, IndentWidth: 8}" -i


documentation: Doxyfile
	-mkdir -p build/doc
	doxygen Doxyfile


install: thready
	cp $^ ~/.local/bin


test: thready
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-nohi.json

testdebug: threadydebug
	valgrind --quiet --leak-check=full --leak-resolution=high ./threadydebug -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./threadydebug -b -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./threadydebug -n makefile-valgrind -j test/p41-ts-nointerarrival-nohi.json


profile: threadydebug
	valgrind --tool=callgrind ./threadydebug -n makefile-callgrind -j test/p41-ts-nointerarrival-nohi.json -t 360000000
