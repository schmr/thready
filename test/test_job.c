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
#include <errno.h>

#include "job.h"

extern int errno;


static void test_job_allocate_ok() {
	job* j = job_init(1, 3, 4, 5);
	assert_non_null(j);
	job_free(j);
}


int setup_job(void** state) {
	job* j = job_init(1, 3, 4, 5);
	*state = j;
	return 0;	// error handling is in job_alloc
}

int teardown_job(void** state) {
	job_free(*state);
	return 0;
}


static void test_job_readable(void** state) {
	job* j = *state;
	assert_int_equal(1, job_get_taskid(j));
	assert_int_equal(3, job_get_starttime(j));
	assert_int_equal(4, job_get_deadline(j));
	assert_int_equal(5, job_get_computation(j));
}


static void test_job_modifyable(void** state) {
	job* j = *state;
	job_set_computation(j, 26);
	assert_int_equal(26, job_get_computation(j));
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_job_allocate_ok),
		cmocka_unit_test_setup_teardown(test_job_readable,
						setup_job,
						teardown_job),
		cmocka_unit_test_setup_teardown(test_job_modifyable,
						setup_job,
						teardown_job),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
