/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "job.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct job {
        JOB_INT taskid;
        JOB_INT starttime;
        JOB_INT overruntime;
        JOB_INT deadline;
        JOB_INT computation;
};

job* job_init(JOB_INT const taskid,
              JOB_INT const starttime,
              JOB_INT const overruntime,
              JOB_INT const deadline,
              JOB_INT const computation) {
        job* j = malloc(sizeof(job));
        if (j) {
                j->taskid = taskid;
                j->starttime = starttime;
                j->overruntime = overruntime;
                j->deadline = deadline;
                j->computation = computation;
                return j;
        } else {
                fprintf(stderr, "error allocating memory for job: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
        }
}

void job_free(job* const j) {
        free(j);
}

JOB_INT job_get_taskid(job* const j) {
        return j->taskid;
}
JOB_INT job_get_starttime(job* const j) {
        return j->starttime;
}
JOB_INT job_get_overruntime(job* const j) {
        return j->overruntime;
}
JOB_INT job_get_deadline(job* const j) {
        return j->deadline;
}
JOB_INT job_get_computation(job* const j) {
        return j->computation;
}

void job_set_computation(job* const j, JOB_INT computation) {
        j->computation = computation;
}
