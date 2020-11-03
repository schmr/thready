.PHONY: all clean format benchmark install test profile documentation unittest integrationtest coverage

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)

cc := gcc
incdirs := -Iinc
ccargscommon := -DVERSION=\"$(GIT_VERSION)\" -std=c99 -Wall -Wextra -pedantic ${incdirs}
ccargsdebugthirdparty := ${ccargscommon} -Werror -march=native -O0 -g -c
ccargsdebug := ${ccargsdebugthirdparty} -fprofile-arcs -ftest-coverage -fPIC
ccargscentos := ${ccargscommon} -march=native -O3 -s
linkargsdebug := -g -lgcov

modules := main pqueue parg rnd selist stats task ts job json jobgen jobq pqueue eventloop dump
src := $(addsuffix .c, $(addprefix src/, ${modules}))
obj := $(addsuffix .o, ${modules})


all: thready test

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


thready: ${src}
	${cc} ${ccargscentos} -o $@ $^ -lm


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
	valgrind --quiet --leak-check=full --leak-resolution=high --exit-on-first-error=yes --log-file=/dev/null ./test_all


integrationtest: thready
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -n makefile-valgrind -j test/p41-ts-nointerarrival-nohi.json
	valgrind --quiet --leak-check=full --leak-resolution=high ./thready -b -n makefile-valgrind -j test/p41-ts-nointerarrival-0.5hi.json

# Coverage

coverage: test_all
	./test_all && gcovr -r . -e inc/rnd.h -e src/pqueue.c -e src/json.c -e src/selist.c -s

# Performance test

profile: threadydebug
	valgrind --tool=callgrind ./threadydebug -n makefile-callgrind -j test/p41-ts-nointerarrival-nohi.json -t 360000000

benchmark: thready-performance-benchmark.csv
thready-performance-benchmark.csv: thready
	seq 30 | parallel --results $@ --eta -j1 './$< -m 36000000 -s {} | sed -e "s/.* \([0-9]\+\) events/\1/"'
#fast2.csv: fast2
#	seq 3600000 1000000 36000000 | parallel --results $@ --eta -j3 './$< {} 0'

# Documentation

format:
	find . \( -name \*.c -or -name \*.h \) | xargs -n12 -P4 clang-format -style="{BasedOnStyle: Chromium, IndentWidth: 8}" -i


documentation: Doxyfile
	-mkdir -p build/doc
	doxygen Doxyfile
