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
	struct jobgen_parameters* p;
	jobgen* jg;
        eventloop* evl;
};


// char const golden[] = "{\"now\":0,\"jobs\":[[2,0,16,1],[5,0,94,1],[0,51,86,1],[1,0,39,1],[6,0,87,1],[-1,39,71,1],[0,0,35,1],[3,0,74,1],[4,0,27,1],[7,0,91,1]]}";


struct ts_random_parameters const EDFok = {
        .p_l = 10,
        .p_u = 100,
        .dtpr_l = 1.0, /* d_i = p_i */
        .dtpr_u = 1.0,
        .u_l = 0.1,
        .u_u = 0.9,
        .z_l = 1,
        .z_u = 1,
        .maxcriticality = 1
};


int setup_eventloop_valid_edf(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	if (!s) { return 1; }

	s->tsy = ts_init();
	ts_generate_uunifastint(&EDFok, 13213, 8, 1.0f, s->tsy);

	s->p = calloc(1, sizeof(struct jobgen_parameters));
	if (s->p) {
		s->p->missionduration = 3600000;
		s->p->interarrivalfactor = 1.5f;
		s->p->errorprobability = 0.1f;
		s->p->repetitions = 0;
	} else {
		return 1;
	}
	s->jg = jobgen_init(s->tsy, s->p, 12312, true);
        s->evl = eventloop_init(s->jg, true);

	*state = s;
	return 0;
}


int setup_eventloop_invalid_edf(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	if (!s) { return 1; }

	s->tsy = ts_init();
	ts_generate_uunifastint(&EDFok, 13213, 8, 1.8f, s->tsy);

	s->p = calloc(1, sizeof(struct jobgen_parameters));
	if (s->p) {
		s->p->missionduration = 3600000;
		s->p->interarrivalfactor = 1.5f;
		s->p->errorprobability = 0.1f;
		s->p->repetitions = 3;
	} else {
		return 1;
	}
	s->jg = jobgen_init(s->tsy, s->p, 12312, true);
        s->evl = eventloop_init(s->jg, true);

	*state = s;
	return 0;
}

int teardown_eventloop(void** state) {
	struct state* s = *state;
        eventloop_free(s->evl);
	jobgen_free(s->jg);
	ts_free(s->tsy);
	free(s->p);
	free(s);
	return 0;
}


static void test_eventloop_persistent(void** state) {
	struct state* s = *state;
	assert_non_null(s->evl);
}


static void test_eventloop_edf_valid_runs_ok(void** state) {
	struct state* s = *state;
        eventloop_result r = eventloop_run(s->evl, 0, 0);
        assert_int_equal(r, EVL_OK);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_edf_invalid_runs_deadlinemiss(void** state) {
	struct state* s = *state;
        eventloop_result r = eventloop_run(s->evl, 0, 0);
        assert_int_equal(r, EVL_DEADLINEMISS);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_stepable(void** state) {
	struct state* s = *state;
	
	eventloop_result r;
        while ((r = eventloop_run(s->evl, 1, 0))) {
		assert_int_equal(r, EVL_CONT);
	}
	assert_int_equal(r, EVL_OK);
	eventloop_print_result(s->evl, r);
}


/*
static void test_eventloop_dump_valid(void** state) {
	struct state* s = *state;

	FILE* stream = fopen("dump.json", "w");
	if (stream) {
		eventloop_dump(s->evl, stream);
		fclose(stream);
	}
	stream = fopen("dump.json", "r");
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

        eventloop_result r = eventloop_run(s->evl, 100, 0);
        assert_int_equal(r, EVL_CONT);

	FILE* stream = fopen("dump.json", "w");
	assert_true(stream);
	eventloop_dump(s->evl, stream);
	fclose(stream);

	stream = fopen("dump.json", "r");
	assert_true(stream);
	eventloop_read_json(s->evl, stream);
	fclose(stream);

	stream = fopen("dump2.json", "w");
	assert_true(stream);
	eventloop_dump(s->evl, stream);
	fclose(stream);

        r = eventloop_run(s->evl, 100, 0);
        assert_int_equal(r, EVL_CONT);
}


static void test_eventloop_breakable(void** state) {
	struct state* s = *state;
	
	eventloop_result r = eventloop_run(s->evl, 0, 300);
	assert_int_equal(r, EVL_CONT);
        while ((r = eventloop_run(s->evl, 1, 0))) {
		assert_int_equal(r, EVL_CONT);
	}
	assert_int_equal(r, EVL_OK);
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
