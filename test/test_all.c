/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

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
                        /* Set up two int* arrays, pointed data is i or 2*i */
                        int* a = calloc(1, sizeof(int));
                        int* b = calloc(1, sizeof(int));
                        if (a && b) {
                                *a = i;
                                *b = 2 * i;
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
        for (int i = 0; i < s->n; i++) {
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
                assert_in_range(x, 0, 2 * s->n);
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

static void test_dump_json_tostream(void** state) {
        struct dumpstate* s = *state;

        FILE* stream = fopen("test_dump_json_tostream.json", "w");
        if (!stream) {
                fail_msg("Could not open file for testing!\n");
        }
        json_printer* jp = dump_json_tostream_init(stream);
        for (int i = 0; i < s->n; i++) {
                int* p = (int*)*(s->src + i);
                dump_json_tostream(jp, (int64_t)(*p));
        }
        dump_json_tostream_free(jp);

        stream = freopen(NULL, "r", stream);
        if (!stream) {
                fail_msg("Could not reopen file for testing!\n");
        }
        char* buf = calloc(2 * s->n, sizeof(char));
        if (!buf) {
                fail_msg("Could not allocate buffer memory!\n");
        }
        *(buf + 2 * s->n - 1) = 0;
        char gld[] = "0,1,2,3,4,5,6,7,8,9";
        size_t rb = fread(buf, sizeof(char), 2 * (s->n) - 1, stream);
        if (!rb) {
                fail_msg("Could not read file for testing!\n");
        }
        assert_string_equal(buf, gld);
        free(buf);
        fclose(stream);
}

int setup_eventloop_valid_edf(void** state) {
        struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-edfok.json", "r");
        if (!tasksystem) {
                return 1;
        }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

        s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true, false);

        *state = s;
        return 0;
}

int setup_eventloop_invalid_edf(void** state) {
        struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-edfnotok.json", "r");
        if (!tasksystem) {
                return 1;
        }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

        s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true, false);

        *state = s;
        return 0;
}

int setup_eventloop_deterministic_edf(void** state) {
        struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-deterministic.json", "r");
        if (!tasksystem) {
                return 1;
        }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

        s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true, false);

        *state = s;
        return 0;
}

int setup_eventloop_deterministic_edf_overrun(void** state) {
        struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-deterministic-overrun.json", "r");
        if (!tasksystem) {
                return 1;
        }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

        s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true, false);

        *state = s;
        return 0;
}

int setup_eventloop_deterministic_edf_overrun_first_allowed(void** state) {
        struct eventloopstate* s = calloc(1, sizeof(struct eventloopstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();
        FILE* tasksystem = fopen("test/ts-deterministic-overrun.json", "r");
        if (!tasksystem) {
                return 1;
        }
        ts_read_json(s->tsy, tasksystem);
        fclose(tasksystem);

        s->jg = jobgen_init(s->tsy, 12312, true);
        s->evl = eventloop_init(s->jg, true, true);

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

static void test_eventloop_run_speed(void** state) {
        struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 27, 1, false);
        assert_int_equal(r, EVL_OK);
        r = eventloop_run(s->evl, 87, 2, false);
        assert_int_equal(r, EVL_OK);
}

static void test_eventloop_run_pass(void** state) {
        struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 27, 1, false);
        assert_int_equal(r, EVL_OK);
        r = eventloop_run(s->evl, 11, 1, false);
        assert_int_equal(r, EVL_PASS);
        eventloop_print_result(s->evl, r);
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

static void test_eventloop_edf_deterministic_cant_overrun(void** state) {
        struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 9273, 1, true);
        assert_int_equal(r, EVL_OK);
        eventloop_print_result(s->evl, r);
}

static void test_eventloop_edf_deterministic_overrun(void** state) {
        struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 9273, 1, true);
        assert_int_equal(r, EVL_OVERRUN);
        eventloop_print_result(s->evl, r);
}

static void test_eventloop_edf_deterministic_overrun_first_allowed(void** state) {
        struct eventloopstate* s = *state;
        eventloop_result r = eventloop_run(s->evl, 7, 1, true);
        assert_int_equal(r, EVL_OK);
        eventloop_print_result(s->evl, r);
        r = eventloop_run(s->evl, 21, 1, true);
        assert_int_equal(r, EVL_OVERRUN);
        eventloop_print_result(s->evl, r);
}

static void test_eventloop_stepable(void** state) {
        struct eventloopstate* s = *state;

        eventloop_result r;
        const LargestIntegralType expected[] = {EVL_OK, EVL_PASS};
        for (int i = 0; i < 153; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                // assert_int_equal(r, EVL_OK);
                assert_in_set(r, expected, 2);
        }
        eventloop_print_result(s->evl, r);
}

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
                char golden[] =
                    "{\"now\":0,\"jobs\":[[19,14,22,21,3],[19,0,8,7,3]]}";
                int len;
                len = fread(&block, sizeof(char), 1024, stream);
                fclose(stream);
                assert_memory_equal(&block, &golden, len);
        }
}

static void test_eventloop_read_json_continues(void** state) {
        struct eventloopstate* s = *state;

        eventloop_result r = eventloop_run(s->evl, 100, 1, false);
        assert_int_equal(r, EVL_OK);

        FILE* stream = fopen("test-eventloop-read.json", "w");
        assert_true(stream);
        eventloop_dump(s->evl, stream);
        fclose(stream);

        stream = fopen("test-eventloop-read2.json", "w");
        assert_true(stream);
        eventloop_dump(s->evl, stream);
        fclose(stream);

        // Assert that dumping twice results in same file content
        char block[1024];
        char golden[] =
            "{\"now\":100,\"jobs\":[[19,112,120,119,3],[19,98,106,105,1]]}";
        int len;

        stream = fopen("test-eventloop-read.json", "r");
        len = fread(&block, sizeof(char), 1024, stream);
        fclose(stream);
        assert_memory_equal(&block, &golden, len);

        stream = fopen("test-eventloop-read.json", "r");
        len = fread(&block, sizeof(char), 1024, stream);
        fclose(stream);
        assert_memory_equal(&block, &golden, len);

        eventloop_free(s->evl);
        s->evl = eventloop_init(s->jg, false, false);

        stream = fopen("test-eventloop-read.json", "r");
        assert_true(stream);
        eventloop_read_json(s->evl, stream);
        fclose(stream);

        r = eventloop_run(s->evl, 200, 1, false);
        assert_int_equal(r, EVL_OK);
}

static void test_eventloop_breakable(void** state) {
        struct eventloopstate* s = *state;

        eventloop_result r = eventloop_run(s->evl, 300, 1, false);
        assert_int_equal(r, EVL_OK);
        const LargestIntegralType expected[] = {EVL_OK, EVL_PASS};
        for (int i = 0; i < 353; i++) {
                r = eventloop_run(s->evl, i, 1, false);
                assert_in_set(r, expected, 2);
        }
        eventloop_print_result(s->evl, r);
}

static void test_eventloop_now_equals_breaktime(void** state) {
        struct eventloopstate* s = *state;
        int breaktime = 823;

        eventloop_result r = eventloop_run(s->evl, breaktime, 1, false);
        assert_int_equal(r, EVL_OK);
        EVL_INT now = eventloop_get_now(s->evl);
        assert_int_equal(now, breaktime);
}

static void test_job_allocate_ok() {
        job* j = job_init(1, 3, 4, 5, 6);
        assert_non_null(j);
        job_free(j);
}

int setup_job(void** state) {
        job* j = job_init(1, 3, 6, 4, 5);
        *state = j;
        return 0;  // error handling is in job_alloc
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
        if (!s) {
                return 1;
        }

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

int setup_jobgen_deterministic(void** state) {
        struct jobgenstate* s = calloc(1, sizeof(struct jobgenstate));
        if (!s) {
                return 1;
        }

        s->tsy = ts_init();

        FILE* stream = fopen("test/ts-deterministic.json", "r");
        if (stream) {
                ts_read_json(s->tsy, stream);
                fclose(stream);
        } else {
                return 1;
        }

        s->jg = jobgen_init(s->tsy, 978382, true);

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

static void test_jobgen_dump(void** state) {
        struct jobgenstate* s = *state;

        void** dst;
        int dumped_jobs = jobgen_dump(s->jg, &dst);
        job** jp = (job**)dst;
        assert_int_equal(dumped_jobs, 1);
        assert_int_equal(job_get_starttime(*jp), 0);
        free(dst);

        job* j = jobgen_rise(s->jg);
        assert_int_equal(job_get_starttime(j), 0);
        job_free(j);

        dumped_jobs = jobgen_dump(s->jg, &dst);
        jp = (job**)dst;
        assert_int_equal(dumped_jobs, 1);
        assert_int_equal(job_get_starttime(*jp), 7);
        free(dst);
}

static void test_jobgen_set_simtime(void** state) {
        struct jobgenstate* s = *state;

        jobgen_free(s->jg);
        s->jg = jobgen_init(s->tsy, 129371, false);
        JOB_INT simtime = 9001;
        jobgen_set_simtime(s->jg, &simtime, 1);
        jobgen_refill_all(s->jg);

        void** dst;
        int dumped_jobs = jobgen_dump(s->jg, &dst);
        job** jp = (job**)dst;
        assert_int_equal(dumped_jobs, 1);
        assert_int_equal(job_get_starttime(*jp), 9001);
        free(dst);
}

static void test_jobgen_get_tasksystem(void** state) {
        struct jobgenstate* s = *state;

        task* t = ts_get_by_pos(s->tsy, 0);
        ts const* tsyjg = jobgen_get_tasksystem(s->jg);
        task* tjg = ts_get_by_pos(tsyjg, 0);
        assert_int_equal(task_get_id(t), task_get_id(tjg));
        assert_int_equal(task_get_period(t), task_get_period(tjg));
        assert_int_equal(task_get_reldead(t), task_get_reldead(tjg));
        assert_int_equal(task_get_comp(t, 0), task_get_comp(tjg, 0));
}

static void test_jobgen_replace_jobq(void** state) {
        struct jobgenstate* s = *state;
        jobq* emptyjq = jobq_init();
        jobgen_replace_jobq(s->jg, emptyjq);
        job* j = jobgen_rise(s->jg);
        assert_null(j);
}

static void test_jobgen_rise(void** state) {
        struct jobgenstate* s = *state;
        for (int i = 0; i < 10; i++) {
                job* j = jobgen_rise(s->jg);
                assert_in_range(job_get_computation(j), 1, 10);
                assert_in_range(job_get_taskid(j) + 1, 0, 6);
                job_free(j);
        }
}

static void test_jobgen_rise_deterministic(void** state) {
        struct jobgenstate* s = *state;
        for (int i = 0; i < 1024; i++) {
                job* j = jobgen_rise(s->jg);
                assert_int_equal(job_get_computation(j), 3);
                assert_int_equal(job_get_taskid(j), 19);
                int starttime = i * 7;
                int deadline = starttime + 7;
                int dummy_overruntime = deadline + 1;
                assert_int_equal(job_get_starttime(j), starttime);
                assert_int_equal(job_get_deadline(j), deadline);
                assert_int_equal(job_get_overruntime(j), dummy_overruntime);
                job_free(j);
        }
}

static void test_jobgen_refill_all_equals_init(void** state) {
        struct jobgenstate* statejg = *state;
        ts* tsy = ts_init();
        assert_non_null(tsy);

        FILE* stream = fopen("test/ts.json", "r");
        assert_non_null(stream);
        ts_read_json(tsy, stream);
        fclose(stream);

        jobgen* s = jobgen_init(tsy, 12312, false);
        jobgen_refill_all(s);
        // TODO: Test that both states are equal, maybe can get away with
        // comparing memory range?
        for (int k = 0; k < ts_length(tsy); k++) {
                job* jrefill = jobgen_rise(s);
                job* jinit = jobgen_rise(statejg->jg);
                assert_int_equal(job_get_taskid(jinit),
                                 job_get_taskid(jrefill));
                job_free(jrefill);
                job_free(jinit);
        }
        ts_free(tsy);
        jobgen_free(s);
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
        free(dump);
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

        for (int i = 0; i < 5; i++) {
                task_set_comp(t, 5 * i, i);
                assert_int_equal(task_get_comp(t, i), 5 * i);
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
                                            setup_task, teardown_task),
            cmocka_unit_test(test_ts_allocate_ok),
            cmocka_unit_test_setup_teardown(test_ts_read_json_valid, setup_ts,
                                            teardown_ts),
            cmocka_unit_test(test_job_allocate_ok),
            cmocka_unit_test_setup_teardown(test_job_readable, setup_job,
                                            teardown_job),
            cmocka_unit_test_setup_teardown(test_job_modifyable, setup_job,
                                            teardown_job),
            cmocka_unit_test_setup_teardown(test_jobgen_persistent,
                                            setup_jobgen, teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_rise, setup_jobgen,
                                            teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_set_simtime,
                                            setup_jobgen_deterministic,
                                            teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_get_tasksystem,
                                            setup_jobgen_deterministic,
                                            teardown_jobgen),
            cmocka_unit_test_setup_teardown(
                test_jobgen_dump, setup_jobgen_deterministic, teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_replace_jobq,
                                            setup_jobgen_deterministic,
                                            teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_rise_deterministic,
                                            setup_jobgen_deterministic,
                                            teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_refill_all_equals_init,
                                            setup_jobgen, teardown_jobgen),
            cmocka_unit_test_setup_teardown(test_jobgen_dump_valid,
                                            setup_jobgen, teardown_jobgen),
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
            cmocka_unit_test_setup_teardown(
                test_jobq_dump_not_null, setup_jobqstate, teardown_jobqstate),
            cmocka_unit_test_setup_teardown(
                test_dump_merge_valid, setup_dumpstate, teardown_dumpstate),
            cmocka_unit_test_setup_teardown(
                test_dump_uniq_valid, setup_dumpstate, teardown_dumpstate),
            cmocka_unit_test_setup_teardown(
                test_dump_json_tostream, setup_dumpstate, teardown_dumpstate),
            cmocka_unit_test_setup_teardown(test_eventloop_persistent,
                                            setup_eventloop_valid_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_edf_valid_runs_ok,
                                            setup_eventloop_valid_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_now_equals_breaktime,
                                            setup_eventloop_valid_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(
                test_eventloop_edf_invalid_runs_deadlinemiss,
                setup_eventloop_invalid_edf, teardown_eventloop),
            cmocka_unit_test_setup_teardown(
                test_eventloop_edf_deterministic_overrun,
                setup_eventloop_deterministic_edf_overrun, teardown_eventloop),
            cmocka_unit_test_setup_teardown(
                test_eventloop_edf_deterministic_overrun_first_allowed,
                setup_eventloop_deterministic_edf_overrun_first_allowed, teardown_eventloop),
            cmocka_unit_test_setup_teardown(
                test_eventloop_edf_deterministic_cant_overrun,
                setup_eventloop_deterministic_edf, teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_run_speed,
                                            setup_eventloop_deterministic_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_run_pass,
                                            setup_eventloop_deterministic_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_stepable,
                                            setup_eventloop_valid_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_dump_valid,
                                            setup_eventloop_deterministic_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_read_json_continues,
                                            setup_eventloop_deterministic_edf,
                                            teardown_eventloop),
            cmocka_unit_test_setup_teardown(test_eventloop_breakable,
                                            setup_eventloop_valid_edf,
                                            teardown_eventloop),
        };
        return cmocka_run_group_tests(tests, NULL, NULL);
}
