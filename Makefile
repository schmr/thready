.PHONY: all clean format benchmark install test profile documentation unittest integrationtest coverage

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

cc := gcc
incdirs := -Iinc
ccargscommon := -DVERSION=\"$(GIT_VERSION)\" -std=c99 -Wall -Wextra -pedantic ${incdirs}
ccargsdebugthirdparty := ${ccargscommon} -Werror -march=native -O0 -g -c
ccargsdebug := ${ccargsdebugthirdparty} -fprofile-arcs -ftest-coverage -fPIC -fsanitize=address
ccargscentos := ${ccargscommon} -march=native -Os -g
ccargscentosopt := ${ccargscommon} -march=native -O3 -s
linkargsdebug := -g -lgcov -lasan

modules := main pqueue parg rnd selist stats task ts job json jobgen jobq pqueue eventloop dump
src := $(addsuffix .c, $(addprefix src/, ${modules}))
obj := $(addsuffix .o, ${modules})


all: thready coverage benchmark

# Disable coverage for thirdparty code
pqueue.o: src/pqueue.c
	${cc} ${ccargsdebugthirdparty} $<
parg.o: src/parg.c
	${cc} ${ccargsdebugthirdparty} $<
selist.o: src/selist.c
	${cc} ${ccargsdebugthirdparty} $<
rnd.o: src/rnd.c
	${cc} ${ccargsdebugthirdparty} $<
json.o: src/json.c
	${cc} ${ccargsdebugthirdparty} $<


%.o: src/%.c
	${cc} ${ccargsdebug} $<


threadydebug: ${obj}
	${cc} -o $@ $^ ${linkargsdebug} -lm

threadyprofile: ${src}
	${cc} ${ccargscommon} -march=native -O0 -g -fprofile-arcs -o $@ $^ -lm


thready: ${src}
	${cc} ${ccargscentos} -o $@ $^ -lm

threadyopt: ${src}
	${cc} ${ccargscentosopt} -o $@ $^ -lm


clean:
	-rm *.o *.gcno *.gcda
	-rm thready threadydebug
	-rm thready-performance-benchmark.csv
	-rm *_dump.json
	-rm test-eventloop-*.json
	-rm test_*
	-rm vgcore.* core.*


install: thready
	cp $^ ~/.local/bin

# Unit test

test_%.o: test/test_%.c
	${cc} ${ccargsdebug} $<


# For coverage it is nice to have a single test executable for all tests
test_all: test_all.o ts.o task.o selist.o rnd.o stats.o json.o job.o jobgen.o jobq.o pqueue.o eventloop.o dump.o stats.o
	${cc} -o $@ $^ ${linkargsdebug} -lcmocka -lm


unittest: test_all
	./test_all


integrationtest: thready
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-nohi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -b -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json

# Coverage

coverage: test_all
	./test_all && gcovr --fail-under-line 100.0 -r . -e inc/rnd.h -e src/pqueue.c -e src/main.c -e src/json.c -e src/selist.c -e test/test_all.c -s

# Performance test

profile: threadyprofile
	valgrind --tool=callgrind ./$< -n makefile-callgrind -j test/p41-ts-nointerarrival-nohi.json -t 360000000

PYTHON := python3.8
benchmark: thready-performance-benchmark.csv
thready-performance-benchmark.csv: thready test/p41-ts-nointerarrival-nohi.json test/check_performance.py
	seq 30 | parallel --results $@ --eta -j1 './$< -n makefile-benchmark -j test/p41-ts-nointerarrival-nohi.json -t 360000000  -z {} | sed -e "s/.* \([0-9]\+\) events .*/\1/"' && ${python} test/check_performance.py

# Documentation

format:
	find . \( -name \*.c -or -name \*.h \) | xargs -n12 -P4 clang-format -style="{BasedOnStyle: Chromium, IndentWidth: 8, IncludeBlocks: Preserve}" -i


documentation: Doxyfile
	-mkdir -p build/doc
	doxygen Doxyfile
