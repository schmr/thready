/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include <stdbool.h>
#include "eventloop.h"


struct state {
	ts* tsy;
	jobgen* jg;
        eventloop* evl;
};


int setup_eventloop_valid_edf(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	if (!s) { return 1; }

	s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-edfok.json", "r");
        if (!tasksystem) { return 1; }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

	s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true);

	*state = s;
	return 0;
}


int setup_eventloop_invalid_edf(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	if (!s) { return 1; }

	s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-edfnotok.json", "r");
        if (!tasksystem) { return 1; }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

	s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true);

	*state = s;
	return 0;
}

int teardown_eventloop(void** state) {
	struct state* s = *state;
        eventloop_free(s->evl);
	jobgen_free(s->jg);
	ts_free(s->tsy);
	free(s);
	return 0;
}


static void test_eventloop_persistent(void** state) {
	struct state* s = *state;
	assert_non_null(s->evl);
}


static void test_eventloop_edf_valid_runs_ok(void** state) {
	struct state* s = *state;
        eventloop_result r = eventloop_run(s->evl, 213, 1, false);
        assert_int_equal(r, EVL_OK);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_edf_invalid_runs_deadlinemiss(void** state) {
	struct state* s = *state;
        eventloop_result r = eventloop_run(s->evl, 9273, 1, false);
        assert_int_equal(r, EVL_DEADLINEMISS);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_stepable(void** state) {
	struct state* s = *state;
	
	eventloop_result r;
        for (int i = 0; i < 153; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                assert_int_equal(r, EVL_OK);
        }
	eventloop_print_result(s->evl, r);
}


/*
static void test_eventloop_dump_valid(void** state) {
	struct state* s = *state;

	FILE* stream = fopen("test-eventloop-dump-valid.json", "w");
	if (stream) {
		eventloop_dump(s->evl, stream);
		fclose(stream);
	}
	stream = fopen("test-eventloop-dump-valid.json", "r");
	if (stream) {
		char block[1024];
		int len;
		len = fread(&block, sizeof(char), 1024, stream);
		fclose(stream);
		assert_memory_equal(&block, &golden, len);
	}
}
*/


static void test_eventloop_read_json_continues(void** state) {
	struct state* s = *state;

        eventloop_result r = eventloop_run(s->evl, 100, 1, false);
        assert_int_equal(r, EVL_OK);

	FILE* stream = fopen("test-eventloop-read.json", "w");
	assert_true(stream);
	eventloop_dump(s->evl, stream);
	fclose(stream);

	stream = fopen("test-eventloop-read.json", "r");
	assert_true(stream);
	eventloop_read_json(s->evl, stream);
	fclose(stream);

	stream = fopen("test-eventloop-read2.json", "w");
	assert_true(stream);
	eventloop_dump(s->evl, stream);
	fclose(stream);

        r = eventloop_run(s->evl, 200, 1, false);
        assert_int_equal(r, EVL_OK);
}


static void test_eventloop_breakable(void** state) {
	struct state* s = *state;
	
	eventloop_result r = eventloop_run(s->evl, 300, 1 , false);
	assert_int_equal(r, EVL_OK);
        for (int i = 0; i < 353; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                assert_int_equal(r, EVL_OK);
        }
	eventloop_print_result(s->evl, r);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_eventloop_persistent,
						setup_eventloop_valid_edf,
						teardown_eventloop),
		cmocka_unit_test_setup_teardown(test_eventloop_edf_valid_runs_ok,
						setup_eventloop_valid_edf,
						teardown_eventloop),
		cmocka_unit_test_setup_teardown(test_eventloop_edf_invalid_runs_deadlinemiss,
						setup_eventloop_invalid_edf,
						teardown_eventloop),
		cmocka_unit_test_setup_teardown(test_eventloop_stepable,
						setup_eventloop_valid_edf,
						teardown_eventloop),
		//cmocka_unit_test_setup_teardown(test_eventloop_dump_valid,
		//				setup_eventloop_valid_edf,
		//				teardown_eventloop),
		cmocka_unit_test_setup_teardown(test_eventloop_read_json_continues,
						setup_eventloop_valid_edf,
						teardown_eventloop),
		cmocka_unit_test_setup_teardown(test_eventloop_breakable,
						setup_eventloop_valid_edf,
						teardown_eventloop),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
