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
#include <stdbool.h>

#include "dump.h"
#include "eventloop.h"
#include "job.h"
#include "jobgen.h"
#include "jobq.h"
#include "task.h"
#include "ts.h"

extern int errno;


struct eventloopstate {
	ts* tsy;
	jobgen* jg;
        eventloop* evl;
};


struct dumpstate {
	void** src;
	void** dst;
	int n;
};


int setup_dumpstate(void** state) {
	struct dumpstate* s = calloc(1, sizeof(struct dumpstate));
	int n = 10;
	s->n = n;
	s->src = calloc(n, sizeof(void*));
	s->dst = calloc(n, sizeof(void*));
	if (s->src && s->dst) {
		for (int i = 0; i < n; i++) {
			int* a = calloc(1, sizeof(int));
			int* b = calloc(1, sizeof(int));
			if (a && b) {
				*a = i;
				*b = 2*i;
				*(s->src + i) = a;
				*(s->dst + i) = b;
			} else {
				return 1;
			}
		}
	} else {
		return 1;
	}
	*state = s;
	return 0;
}

int teardown_dumpstate(void** state) {
	struct dumpstate* s = *state;
	for (int i = 0; i<s->n; i++) {
		free(*(s->src + i));
		free(*(s->dst + i));
	}
	free(s->src);
	free(s->dst);
	free(s);
	return 0;
}


static void test_dump_merge_valid(void** state) {
	struct dumpstate* s = *state;
	void** m = merge(s->src, s->dst, s->n, s->n);
	for (int i = 0; i < s->n * 2; i++) {
		assert_non_null(*(m + i));
		int* p = *(m + i);
		int x = *p;
		assert_in_range(x, 0, 2*s->n);
	}
	free(m);
}


static void test_dump_uniq_valid(void** state) {
	struct dumpstate* s = *state;
	void** m = merge(s->src, s->src, s->n, s->n);
	int len = s->n * 2;
	void** u = calloc(len, sizeof(void*));
	int lenuniq = uniq(m, u, len);
	assert_int_equal(lenuniq, s->n);
	free(m);
	free(u);
}


int setup_eventloop_valid_edf(void** state) {
	struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
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
	struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
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
	struct eventloopstate* s = *state;
        eventloop_free(s->evl);
	jobgen_free(s->jg);
	ts_free(s->tsy);
	free(s);
	return 0;
}


static void test_eventloop_persistent(void** state) {
	struct eventloopstate* s = *state;
	assert_non_null(s->evl);
}


static void test_eventloop_edf_valid_runs_ok(void** state) {
	struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 213, 1, false);
        assert_int_equal(r, EVL_OK);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_edf_invalid_runs_deadlinemiss(void** state) {
	struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 9273, 1, false);
        assert_int_equal(r, EVL_DEADLINEMISS);
	eventloop_print_result(s->evl, r);
}


static void test_eventloop_stepable(void** state) {
	struct eventloopstate* s = *state;
	
	eventloop_result r;
        for (int i = 0; i < 153; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                assert_int_equal(r, EVL_OK);
        }
	eventloop_print_result(s->evl, r);
}


/*
static void test_eventloop_dump_valid(void** state) {
	struct eventloopstate* s = *state;

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
	struct eventloopstate* s = *state;

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
	struct eventloopstate* s = *state;
	
	eventloop_result r = eventloop_run(s->evl, 300, 1 , false);
	assert_int_equal(r, EVL_OK);
        for (int i = 0; i < 353; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                assert_int_equal(r, EVL_OK);
        }
	eventloop_print_result(s->evl, r);
}


static void test_job_allocate_ok() {
	job* j = job_init(1, 3, 4, 5, 6);
	assert_non_null(j);
	job_free(j);
}


int setup_job(void** state) {
	job* j = job_init(1, 3, 6, 4, 5);
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


struct jobgenstate {
	ts* tsy;
	jobgen* jg;
};


int setup_jobgen(void** state) {
	struct jobgenstate* s = calloc(1, sizeof(struct jobgenstate));
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
	struct jobgenstate* s = *state;
	jobgen_free(s->jg);
	ts_free(s->tsy);
	free(s);
	return 0;
}


static void test_jobgen_persistent(void** state) {
	struct jobgenstate* s = *state;
	assert_non_null(s->jg);
}


static void test_jobgen_rise(void** state) {
	struct jobgenstate* s = *state;
	for (int i = 0; i<10; i++) {
		job* j = jobgen_rise(s->jg);
		assert_in_range(job_get_computation(j), 1, 10);
		assert_in_range(job_get_taskid(j) + 1, 0, 6);
		job_free(j);
	}
}


static void test_jobgen_dump_valid(void** state) {
	struct jobgenstate* s = *state;
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


struct jobqstate {
	job* js[2];
	jobq* jq;
};


static void test_jobq_allocate_ok() {
	jobq* jq = jobq_init();
	assert_non_null(jq);
	jobq_free(jq);
}


int setup_jobqstate(void** state) {
	struct jobqstate* s = calloc(1, sizeof(struct jobqstate));
	s->jq = jobq_init();
	s->js[0] = job_init(1, 3, 7, 4, 5);
	s->js[1] = job_init(20, 40, 101, 50, 60);
	*state = s;
	return 0;
}

int teardown_jobqstate(void** state) {
	struct jobqstate* s = *state;
	jobq_free(s->jq);
	free(s);
	return 0;
}


static void test_jobq_insertable_readable(void** state) {
	struct jobqstate* s = *state;
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
	struct jobqstate* s = *state;
	jobq_insert_by(s->jq, s->js[0], job_get_starttime);
	jobq_insert_by(s->jq, s->js[1], job_get_starttime);
	job* j = jobq_peek(s->jq);

	assert_int_equal(3, job_get_starttime(j));
}

static void test_jobq_empty_returns_null(void** state) {
	struct jobqstate* s = *state;
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
	struct jobqstate* s = *state;
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
	assert_int_equal(1, task_get_comp(t, 0));
	assert_int_equal(10, task_get_comp(t, 1));

	t = ts_get_by_id(tsy, 5);
	assert_int_equal(8, task_get_reldead(t));
	assert_int_equal(10, task_get_period(t));
	assert_int_equal(1, task_get_comp(t, 0));
	assert_int_equal(1, task_get_comp(t, 1));
	assert_int_equal(2, task_get_comp(t, 2));
	assert_int_equal(4, task_get_comp(t, 3));
	assert_int_equal(5, task_get_comp(t, 4));
	assert_int_equal(7, task_get_comp(t, 5));

	t = ts_get_by_id(tsy, 3);
	assert_int_equal(12, task_get_reldead(t));
	assert_int_equal(12, task_get_period(t));
	assert_int_equal(1, task_get_comp(t, 0));
	assert_int_equal(1, task_get_comp(t, 1));
	assert_int_equal(2, task_get_comp(t, 2));
	assert_int_equal(9, task_get_comp(t, 3));

}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_task_allocate_ok),
		cmocka_unit_test_setup_teardown(test_task_writable_readable,
						setup_task,
						teardown_task),
		cmocka_unit_test(test_ts_allocate_ok),
		cmocka_unit_test_setup_teardown(test_ts_read_json_valid,
						setup_ts,
						teardown_ts),
		cmocka_unit_test(test_job_allocate_ok),
		cmocka_unit_test_setup_teardown(test_job_readable,
						setup_job,
						teardown_job),
		cmocka_unit_test_setup_teardown(test_job_modifyable,
						setup_job,
						teardown_job),
		cmocka_unit_test_setup_teardown(test_jobgen_persistent,
						setup_jobgen,
						teardown_jobgen),
		cmocka_unit_test_setup_teardown(test_jobgen_rise,
						setup_jobgen,
						teardown_jobgen),
		cmocka_unit_test_setup_teardown(test_jobgen_dump_valid,
						setup_jobgen,
						teardown_jobgen),
		cmocka_unit_test(test_jobq_allocate_ok),
		cmocka_unit_test_setup_teardown(test_jobq_insertable_readable,
						setup_jobqstate,
						teardown_jobqstate),
		cmocka_unit_test_setup_teardown(test_jobq_ordered_by_arrival,
						setup_jobqstate,
						teardown_jobqstate),
		cmocka_unit_test_setup_teardown(test_jobq_empty_returns_null,
						setup_jobqstate,
						teardown_jobqstate),
		cmocka_unit_test_setup_teardown(test_jobq_dump_not_null,
						setup_jobqstate,
						teardown_jobqstate),
		cmocka_unit_test_setup_teardown(test_dump_merge_valid,
						setup_dumpstate,
						teardown_dumpstate),
		cmocka_unit_test_setup_teardown(test_dump_uniq_valid,
						setup_dumpstate,
						teardown_dumpstate),
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
