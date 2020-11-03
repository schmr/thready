/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "eventloop.h"
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "dump.h"
#include "jobq.h"
#include "json.h"
#include "selist.h"

struct eventloop {
        jobgen* jg;
        jobq* pq;
        EVL_INT events_done;
        EVL_INT now;
        JOB_INT jobs_done;
        job* currentjob;
        job* nextjob;
};

eventloop* eventloop_init(jobgen* const jg, bool init) {
        eventloop* evl = calloc(1, sizeof(eventloop));
        if (evl) {
                evl->jg = jg;
                evl->pq = jobq_init();
                if (init) {  // differentiate to support resume from state dump
                        evl->currentjob = jobgen_rise(evl->jg);
                        evl->nextjob = jobgen_rise(evl->jg);
                        assert(job_get_starttime(evl->currentjob) <=
                               job_get_starttime(evl->nextjob));
                        evl->now = job_get_starttime(evl->currentjob);
                        jobq_insert_by(evl->pq, evl->currentjob,
                                       job_get_deadline);
                        evl->jobs_done = 0;
                        evl->events_done = 0;
                }
        } else { // GCOVR_EXCL_START
                fprintf(stderr, "error allocating memory for eventloop\n");
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
        return evl;
}

void eventloop_free(eventloop* evl) {
        job_free(evl->nextjob);
        jobq_free(evl->pq);
        free(evl);
}

EVL_INT eventloop_get_now(eventloop* evl) {
        return evl->now;
}

eventloop_result eventloop_run(eventloop* evl,
                               JOB_INT breaktime,
                               JOB_INT speed,
                               bool overrunbreak) {
        // Initialize local pointers, now is set to starttime of current job
        // by initialization of eventloop, and currentjob is added to scheduler
        // queue.
        job* currentjob = evl->currentjob;
        job* nextjob = evl->nextjob;

        while (evl->now < breaktime) {
                // Assert now is arrival of current job
                JOB_INT arrival = job_get_starttime(nextjob);
                JOB_INT runtime;
                if (arrival < breaktime) {
                        runtime = arrival - evl->now;
                } else {  // absolute breaktime earlier than next arrival
                        runtime = breaktime - evl->now;
                }
                // Check if current task overruns earlier than next task arrival
                currentjob = jobq_peek(evl->pq);
                JOB_INT overruntime;
                if (currentjob) {
                        overruntime = job_get_overruntime(currentjob);
                } else {
                        // If there is no job in scheduler queue,
                        // disable overrun checking with dummy overrun time
                        // which is too late to be considered.
                        overruntime = arrival + 123;
                }
                if (overrunbreak && (overruntime < arrival)) {
                        runtime = overruntime - evl->now;
                }
                while (runtime > 0) {
                        currentjob = jobq_peek(evl->pq);
                        if (!currentjob) {  // No job in scheduler queue
                                break;
                        }
                        JOB_INT deadline = job_get_deadline(currentjob);
                        JOB_INT c = job_get_computation(currentjob);
                        JOB_INT workdelta = runtime * speed;
                        if (workdelta <= c) {  // Spend complete runtime on job
                                evl->now = evl->now + runtime;
                                job_set_computation(currentjob, c - workdelta);
                                runtime = 0;
                        } else {  // Finish job and update runtime budget
                                JOB_INT time_spent =
                                    c / speed;  // truncation is optimistic
                                if (c % speed >
                                    0) {  // conservative; wasting some capacity
                                        runtime -= 1;
                                        evl->now += 1;
                                }
                                evl->now = evl->now + time_spent;
                                runtime = runtime - time_spent;
                                // Free finished job
                                job_free(jobq_pop(evl->pq));
                                evl->jobs_done++;
                        }
                        evl->events_done++;  // Finishing, preempting a job, or
                                             // missing its deadline is counted
                                             // as an event
                        if (evl->now > deadline) {  // Did we miss the deadline
                                                    // of currentjob?
                                evl->currentjob = currentjob;
                                evl->nextjob = nextjob;
                                evl->now = deadline;
                                return EVL_DEADLINEMISS;
                        }
                }
                if ((evl->now == breaktime) ||
                    ((evl->now + runtime) ==
                     breaktime)) {  // Stop prior to arrival as requested
                                    // dealing with remaining time in case of
                                    // empty scheduler queue
                        evl->now = breaktime;  // Equalize now for both cases
                        break;
                }
                if (overrunbreak && (evl->now == overruntime)) {
                        evl->currentjob = currentjob;
                        evl->nextjob = nextjob;
                        return EVL_OVERRUN;
                }
                // Arrival
                evl->now = arrival;
                jobq_insert_by(evl->pq, nextjob, job_get_deadline);
                nextjob = jobgen_rise(evl->jg);
                evl->events_done++;  // Arrival of a job is counted as an event
        }
        evl->currentjob = currentjob;
        evl->nextjob = nextjob;
        return EVL_OK;
}

void eventloop_print_result(eventloop const* const evl,
                            eventloop_result const result) {
        switch (result) {
                case EVL_OK:
                        fprintf(stdout,
                                "%" PRId64 ": End of simulation with %" PRId64
                                " events servicing %" PRId64 " jobs\n",
                                (int64_t)(evl->now),
                                (int64_t)(evl->events_done),
                                (int64_t)(evl->jobs_done));
                        break;
                case EVL_DEADLINEMISS:
                        fprintf(stdout,
                                "%" PRId64 ": Deadline miss after %" PRId64
                                " events servicing %" PRId64 " jobs\n",
                                (int64_t)(evl->now),
                                (int64_t)(evl->events_done),
                                (int64_t)(evl->jobs_done));
                        break;
                case EVL_OVERRUN:
                        fprintf(stdout,
                                "%" PRId64 ": Overrun after %" PRId64
                                " events servicing %" PRId64 " jobs\n",
                                (int64_t)(evl->now),
                                (int64_t)(evl->events_done),
                                (int64_t)(evl->jobs_done));
                        break;
                case EVL_PASS:
                        // Nothing simulated, no knowledge about outcome
                        fprintf(stdout, "%" PRId64 ": Pass simulation\n",
                                (int64_t)(evl->now));
                        break;
                default:
                        break;
        }
}

void eventloop_dump(eventloop const* const evl, FILE* stream) {
        // Get list of unique jobs
        void** dump_jg = (void*)0;
        void** dump_pq = (void*)0;
        int len_jg = jobgen_dump(evl->jg, &dump_jg);
        int len_pq = jobq_dump(evl->pq, &dump_pq);

        int len = len_jg + len_pq;
        void** m = merge(dump_jg, dump_pq, len_jg, len_pq);
        void** u = calloc(len, sizeof(void*));
        int lenuniq = uniq(m, u, len);
        free(m);
        free(dump_jg);
        free(dump_pq);

        // Write JSON to stream
        json_printer* print = dump_json_tostream_init(stream);

        json_print_raw(print, JSON_OBJECT_BEGIN, NULL, 0);
        json_print_raw(print, JSON_KEY, "now", 3);
        dump_json_tostream(print, evl->now);

        json_print_raw(print, JSON_KEY, "jobs", 4);
        json_print_raw(print, JSON_ARRAY_BEGIN, NULL, 0);

        job** jp = (job**)u;
        for (int i = 0; i < lenuniq; i++) {
                job* j = *(jp + i);
                json_print_raw(print, JSON_ARRAY_BEGIN, NULL, 0);

                dump_json_tostream(print, job_get_taskid(j));
                dump_json_tostream(print, job_get_starttime(j));
                dump_json_tostream(print, job_get_overruntime(j));
                dump_json_tostream(print, job_get_deadline(j));
                dump_json_tostream(print, job_get_computation(j));

                json_print_raw(print, JSON_ARRAY_END, NULL, 0);
        }
        free(u);

        json_print_raw(print, JSON_ARRAY_END, NULL, 0);
        json_print_raw(print, JSON_OBJECT_END, NULL, 0);
        dump_json_tostream_free(print);
}

void eventloop_read_json(eventloop* evl, FILE* stream) {
        // This is a partial state restoration, beware!
        struct selist* l = (void*)0;
        json_parser* parser = dump_read_json_init(&l);
        dump_read_json_parse(parser, stream);
        dump_read_json_free(parser);

        int i = 0;

        intmax_t* now = selist_get(l, i++);
        evl->now = *now;

        ts const* tsy = jobgen_get_tasksystem(evl->jg);
        jobq* scheduler = jobq_init();
        jobq* generator = jobq_init();
        JOB_INT* simtimes = calloc(ts_length(tsy), sizeof(JOB_INT));
        if (!simtimes) { //GCOVR_EXCL_START
                fprintf(stderr,
                        "error allocating memory while restoring state\n");
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
        while (i < selist_length(l)) {  // skipped if no job in list due to drop
                // Recreate job from list with knowledge about order,
                // sorry future me/others!
                intmax_t* taskid = selist_get(l, i++);
                intmax_t* starttime = selist_get(l, i++);
                intmax_t* overruntime = selist_get(l, i++);
                intmax_t* deadline = selist_get(l, i++);
                intmax_t* computation = selist_get(l, i++);
                job* j = job_init(*taskid, *starttime, *overruntime, *deadline,
                                  *computation);
                if (*starttime > *now) {
                        jobq_insert_by(generator, j, job_get_starttime);
                        int k = ts_get_pos_by_id(tsy, *taskid);
                        task* t = ts_get_by_id(tsy, *taskid);
                        JOB_INT delta = task_get_period(t) - *computation;
                        // Random duration between two jobs of a task is set to
                        // zero. For seamless restoration a dump of the random
                        // number generator state would be required.
                        *(simtimes + k) = *starttime + *computation + delta;
                } else {
                        jobq_insert_by(scheduler, j, job_get_deadline);
                }
        }
        jobq_free(evl->pq);
        evl->pq = scheduler;
        jobgen_set_simtime(evl->jg, simtimes, ts_length(tsy));
        jobgen_replace_jobq(evl->jg, generator);
        if (i == 1) {  // we dropped everything
                // simtimes are all zero, need to set them to current
                // missiontime
                for (int k = 0; k < ts_length(tsy); k++) {
                        *(simtimes + k) = *now;
                }
                jobgen_set_simtime(evl->jg, simtimes, ts_length(tsy));
                jobgen_refill_all(evl->jg);
        }
        // It is perfectly fine to raise NULL if no job is due because we might
        // already have passed beyond the mission duration, which would prevent
        // the generator from creating new jobs.
        evl->currentjob = jobgen_rise(evl->jg);
        evl->nextjob = jobgen_rise(evl->jg);
        if (evl->currentjob && evl->nextjob) {
                assert(job_get_starttime(evl->currentjob) <=
                       job_get_starttime(evl->nextjob));
                jobq_insert_by(evl->pq, evl->currentjob, job_get_deadline);
        }
        free(simtimes);
}
