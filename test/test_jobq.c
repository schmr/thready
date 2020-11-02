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
#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>

#include "jobq.h"


struct state {
	job* js[2];
	jobq* jq;
};


static void test_jobq_allocate_ok() {
	jobq* jq = jobq_init();
	assert_non_null(jq);
	jobq_free(jq);
}


int setup_state(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	s->jq = jobq_init();
	s->js[0] = job_init(1, 3, 4, 5);
	s->js[1] = job_init(20, 40, 50, 60);
	*state = s;
	return 0;
}

int teardown_state(void** state) {
	struct state* s = *state;
	jobq_free(s->jq);
	free(s);
	return 0;
}


static void test_jobq_insertable_readable(void** state) {
	struct state* s = *state;
	jobq_insert_by(s->jq, s->js[0], job_get_starttime);
	job* j = jobq_peek(s->jq);

	assert_int_equal(1, job_get_taskid(j));
	assert_int_equal(3, job_get_starttime(j));
	assert_int_equal(4, job_get_deadline(j));
	assert_int_equal(5, job_get_computation(j));

	/* For shared teardown */
	jobq_insert_by(s->jq, s->js[1], job_get_starttime);
}

static void test_jobq_ordered_by_arrival(void** state) {
	struct state* s = *state;
	jobq_insert_by(s->jq, s->js[0], job_get_starttime);
	jobq_insert_by(s->jq, s->js[1], job_get_starttime);
	job* j = jobq_peek(s->jq);

	assert_int_equal(3, job_get_starttime(j));
}

static void test_jobq_empty_returns_null(void** state) {
	struct state* s = *state;
	jobq_insert_by(s->jq, s->js[0], job_get_starttime);
	jobq_insert_by(s->jq, s->js[1], job_get_starttime);

	job* j = jobq_pop(s->jq);
	assert_int_equal(3, job_get_starttime(j));
	j = jobq_pop(s->jq);
	assert_int_equal(40, job_get_starttime(j));
	j = jobq_pop(s->jq);
	assert_null(j);

	/* Need to free jobs manually because shared teardown only frees jobs
	 * in jobq.
	 */
	job_free(s->js[0]);
	job_free(s->js[1]);
}

static void test_jobq_dump_not_null(void** state) {
	struct state* s = *state;
	jobq_insert_by(s->jq, s->js[0], job_get_starttime);
	jobq_insert_by(s->jq, s->js[1], job_get_starttime);

	void** dump = (void*)0;
	int len = jobq_dump(s->jq, &dump);
	assert_int_equal(2, len);
	assert_non_null(dump);
	assert_non_null(*dump);
	assert_non_null(*(dump + 1));

	job* j = *dump;
	assert_int_equal(3, job_get_starttime(j));
	j = *(dump + 1);
	assert_int_equal(40, job_get_starttime(j));

	free(dump);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_jobq_allocate_ok),
		cmocka_unit_test_setup_teardown(test_jobq_insertable_readable,
						setup_state,
						teardown_state),
		cmocka_unit_test_setup_teardown(test_jobq_ordered_by_arrival,
						setup_state,
						teardown_state),
		cmocka_unit_test_setup_teardown(test_jobq_empty_returns_null,
						setup_state,
						teardown_state),
		cmocka_unit_test_setup_teardown(test_jobq_dump_not_null,
						setup_state,
						teardown_state),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
