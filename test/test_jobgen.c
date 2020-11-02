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
#include "jobgen.h"


struct state {
	ts* tsy;
	jobgen* jg;
};


int setup_jobgen(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	if (!s) { return 1; }

	s->tsy = ts_init();

        FILE* stream = fopen("test/ts.json", "r");
        if (stream) {
		ts_read_json(s->tsy, stream);
                fclose(stream);
        } else {
		return 1;
	}

	s->jg = jobgen_init(s->tsy, 12312, true);

	*state = s;
	return 0;
}

int teardown_jobgen(void** state) {
	struct state* s = *state;
	jobgen_free(s->jg);
	ts_free(s->tsy);
	free(s);
	return 0;
}


static void test_jobgen_persistent(void** state) {
	struct state* s = *state;
	assert_non_null(s->jg);
}


static void test_jobgen_rise(void** state) {
	struct state* s = *state;
	for (int i = 0; i<10; i++) {
		job* j = jobgen_rise(s->jg);
		assert_in_range(job_get_computation(j), 1, 10);
		assert_in_range(job_get_taskid(j) + 1, 0, 6);
		job_free(j);
	}
}


static void test_jobgen_dump_valid(void** state) {
	struct state* s = *state;
	void** dump = (void*)0;
	int len = jobgen_dump(s->jg, &dump);
	assert_int_equal(3, len);
	
	for (int i = 0; i < len; i++) {
		job** jp = (job**)(dump + i);
		job* j = *jp;
		assert_in_range(job_get_computation(j), 1, 10);
		assert_in_range(job_get_taskid(j) + 1, 0, 6);
	}
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_jobgen_persistent,
						setup_jobgen,
						teardown_jobgen),
		cmocka_unit_test_setup_teardown(test_jobgen_rise,
						setup_jobgen,
						teardown_jobgen),
		cmocka_unit_test_setup_teardown(test_jobgen_dump_valid,
						setup_jobgen,
						teardown_jobgen),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
