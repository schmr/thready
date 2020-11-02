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

#include "ts.h"


static void test_ts_allocate_ok() {
	ts* tsy = ts_init();
	assert_non_null(tsy);
	ts_free(tsy);
}


int setup_ts(void** state) {
	ts* tsy = ts_init();
	*state = tsy;
	return 0;
}

int teardown_ts(void** state) {
	ts_free(*state);
	return 0;
}


static void test_ts_generateable(void** state) {
	ts* t = *state;
        struct ts_random_parameters EDFok = {
            .p_l = 10,
            .p_u = 100,
            .dtpr_l = 1.0, /* d_i = p_i */
            .dtpr_u = 1.0,
            .u_l = 0.1,
            .u_u = 0.9,
            .z_l = 1,
            .z_u = 1,
            .maxcriticality = 1};

	ts_generate_uunifastint(&EDFok, 13213, 8, 1.0f, t);
	assert_in_range(ts_length(t), 1, 9);
}

static void test_ts_read_json_valid(void** state) {
	ts* tsy = *state;

        FILE* stream = fopen("test/ts.json", "r");
	assert_true(stream);
        if (stream) {
		ts_read_json(tsy, stream);
                fclose(stream);
        }
	int len = ts_length(tsy);
	assert_int_equal(len, 3);

	task* t = ts_get_by_id(tsy, -1);
	assert_int_equal(20, task_get_reldead(t));
	assert_int_equal(20, task_get_period(t));
	assert_int_equal(1, task_get_maxcrit(t));
	assert_int_equal(10, task_get_comp(t, 0));

	t = ts_get_by_id(tsy, 5);
	assert_int_equal(8, task_get_reldead(t));
	assert_int_equal(10, task_get_period(t));
	assert_int_equal(3, task_get_maxcrit(t));
	assert_int_equal(1, task_get_comp(t, 0));
	assert_int_equal(2, task_get_comp(t, 1));
	assert_int_equal(7, task_get_comp(t, 2));

	t = ts_get_by_id(tsy, 3);
	assert_int_equal(12, task_get_reldead(t));
	assert_int_equal(12, task_get_period(t));
	assert_int_equal(2, task_get_maxcrit(t));
	assert_int_equal(1, task_get_comp(t, 0));
	assert_int_equal(9, task_get_comp(t, 1));

}


static void test_ts_dump_valid(void** state) {
	ts* tsy = *state;

        FILE* stream = fopen("test/ts.json", "r");
	assert_true(stream);
        if (stream) {
		ts_read_json(tsy, stream);
                fclose(stream);
        }
	
	stream = fopen("ts.json", "w");
	assert_true(stream);
	if (stream) {
		ts_dump(tsy, stream);
		fclose(stream);
	}
	stream = fopen("ts.json", "r");
	if (stream) {
		char golden[] = "[[5,10,8,3,1,2,7],[3,12,12,2,1,9],[-1,20,20,1,10]]";
		char block[1024];
		int len;
		len = fread(&block, sizeof(char), 1024, stream);
		fclose(stream);
		assert_memory_equal(&block, &golden, len);
	}
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_ts_allocate_ok),
		cmocka_unit_test_setup_teardown(test_ts_generateable,
						setup_ts,
						teardown_ts),
		cmocka_unit_test_setup_teardown(test_ts_read_json_valid,
						setup_ts,
						teardown_ts),
		cmocka_unit_test_setup_teardown(test_ts_dump_valid,
						setup_ts,
						teardown_ts),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
