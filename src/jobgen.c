/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "jobgen.h"
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jobq.h"
#include "rnd.h"
#include "stats.h"
#include "task.h"
#include "ts.h"

struct jobgen {
        ts const* tsy;

        jobq* jq;
        JOB_INT* simtime_state;
        rnd_pcg_t** pcg;
        bool had_overrun;
};

static void refill_generator(jobgen* jg, TASK_INT taskid);

jobgen* jobgen_init(ts const* const tasksystem, uint32_t seed, bool refill) {
        jobq* jque = jobq_init();
        JOB_INT* simtime_state = calloc(ts_length(tasksystem), sizeof(JOB_INT));
        jobgen* jgen = calloc(1, sizeof(jobgen));

        // Maybe flatten error handling with goto?
        if (jgen && simtime_state) {
                jgen->had_overrun = false;
                jgen->jq = jque;
                jgen->tsy = tasksystem;
                jgen->simtime_state = simtime_state;
                jgen->pcg = calloc(1, sizeof(rnd_pcg_t*));
                if (jgen->pcg) {
                        *(jgen->pcg) = calloc(1, sizeof(rnd_pcg_t));
                        if (!*(jgen->pcg)) {  // GCOVR_EXCL_START
                                fprintf(stderr,
                                        "error allocating memory for jobgen\n");
                                exit(EXIT_FAILURE);
                        }
                } else {
                        fprintf(stderr, "error allocating memory for jobgen\n");
                        exit(EXIT_FAILURE);
                }  // GCOVR_EXCL_STOP
                rnd_pcg_seed(*(jgen->pcg), seed);
        } else {  // GCOVR_EXCL_START
                fprintf(stderr, "error allocating memory for jobgen\n");
                exit(EXIT_FAILURE);
        }  // GCOVR_EXCL_STOP

        if (refill) {
                for (int k = 0; k < ts_length(tasksystem); k++) {
                        task* t = ts_get_by_pos(tasksystem, k);
                        refill_generator(jgen, task_get_id(t));
                }
        }

        return jgen;
}

void jobgen_free(jobgen* jg) {
        free(jg->simtime_state);
        jobq_free(jg->jq);
        free(*(jg->pcg));
        free(jg->pcg);
        free(jg);
}

static float uniform3(rnd_pcg_t** pcg, task* t) {
        float y = uniformf(pcg, 0.0f, 1.0f);
        float p0 = task_get_prob(t, 0);
        float p1 = task_get_prob(t, 1);

        int segment = 0;
        if (y > p0 + p1) {
                segment = 2;
        } else if (y > p0) {
                segment = 1;
        } else {
                segment = 0;
        }
        TASK_INT clow = task_get_comp(t, 2 * segment);
        TASK_INT chigh = task_get_comp(t, 2 * segment + 1);

        return uniformf(pcg, (float)clow, (float)chigh);
}

static void refill_generator(jobgen* jg, TASK_INT taskid) {
        int k = ts_get_pos_by_id(jg->tsy, taskid);
        task* t = ts_get_by_id(jg->tsy, taskid);

        JOB_INT simtime = *(jg->simtime_state + k);
        TASK_INT period = task_get_period(t);
        TASK_INT reldead = task_get_reldead(t);
        float interarrivalfactor = task_get_beta(t);

        JOB_INT rho = exponential(jg->pcg, interarrivalfactor) * period;
        JOB_INT gamma = ceil(uniform3(jg->pcg, t));
        assert(gamma > 0);
        JOB_INT alpha = simtime;
        simtime = simtime + period + rho;
        JOB_INT deadline = alpha + reldead;

        JOB_INT c1 = task_get_comp(t, 1);
        if ((gamma > c1) && (!jg->had_overrun)) {
                jg->had_overrun = true;
                fprintf(stdout,
                        "Overflowing job of task %" PRId64
                        " arrives at %" PRId64 " with deadline at %" PRId64
                        " and computation of %" PRId64
                        " which is an overrun of %" PRId64 " \n",
                        taskid, alpha, deadline, gamma, (gamma - c1));
        }

        // If a non-zero computation budget is defined and we can reach it by
        // chance the task is a high criticality task and can overrun.
        JOB_INT c2 = task_get_comp(t, 2);
        float p0 = task_get_prob(t, 0);
        JOB_INT overruntime;
        if ((c2 > 0) && (p0 < 1.0f)) {
                /* Overrun time is relative, don't know absolute times until simulation */
                overruntime = c1 + 1;
        } else {
                // Overruntime beyond computation is not put to use by eventloop.
                // This effectively sets the overruntime to "not available" or
                // irrelevant.
                overruntime = gamma + 1;
        }

        job* job = job_init(taskid, alpha, overruntime, deadline, gamma);

        *(jg->simtime_state + k) = simtime;
        jobq_insert_by(jg->jq, job, job_get_starttime);
}

job* jobgen_rise(jobgen* jg) {
        job* j = jobq_pop(jg->jq);
        if (j) {  // mission still running, generator not exhausted
                refill_generator(jg, job_get_taskid(j));
        }
        return j;
}
void jobgen_refill_all(jobgen* jg) {
        int n = ts_length(jg->tsy);
        for (int i = 0; i < n; i++) {
                task* t = ts_get_by_pos(jg->tsy, i);
                TASK_INT tid = task_get_id(t);
                refill_generator(jg, tid);
        }
}

void jobgen_set_simtime(jobgen* jg, JOB_INT* simtimes, int len) {
        int tasks = ts_length(jg->tsy);
        if (tasks != len) {  // GCOVR_EXCL_START
                fprintf(stderr, "can't seed simtimes due to length mismatch\n");
                exit(EXIT_FAILURE);
        }  // GCOVR_EXCL_STOP
        for (int i = 0; i < tasks; i++) {
                *(jg->simtime_state + i) = *(simtimes + i);
        }
}

ts const* jobgen_get_tasksystem(jobgen const* const jg) {
        return jg->tsy;
}

int jobgen_dump(jobgen const* const jg, void*** dst) {
        return jobq_dump(jg->jq, dst);
}

void jobgen_replace_jobq(jobgen* const jgen, jobq* const jq) {
        jobq_free(jgen->jq);
        jgen->jq = jq;
}
