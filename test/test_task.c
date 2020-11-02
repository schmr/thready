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

#include "task.h"

extern int errno;


static void test_task_allocate_ok() {
	task* t = task_init(5);
	assert_non_null(t);
	task_free(t);
}


int setup_task(void** state) {
	task* t = task_init(5);
	*state = t;
	return 0;
}

int teardown_task(void** state) {
	task_free(*state);
	return 0;
}


static void test_task_writable_readable(void** state) {
	task* t = *state;
	task_set_id(t, 1);
	assert_int_equal(1, task_get_id(t));

	task_set_period(t, 90);
	assert_int_equal(task_get_period(t), 90);

	task_set_reldead(t, 100);
	assert_int_equal(task_get_reldead(t), 100);

	for(int i=0; i<5; i++) {
		task_set_comp(t, 5*i, i);
		assert_int_equal(task_get_comp(t, i), 5*i);
	}
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_task_allocate_ok),
		cmocka_unit_test_setup_teardown(test_task_writable_readable,
						setup_task,
						teardown_task),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
