/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "task.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int errno;

struct task {
        TASK_INT id;
        TASK_INT period;
        TASK_INT reldead;
        TASK_INT comp[TASK_NUM_COMP];
        float prob[TASK_NUM_PROB];
        float beta;  // interarrival time distribution factor
};

task* task_init() {
        task* t = calloc(1, sizeof(task));
        if (!t) {
                fprintf(stderr, "error allocating memory for task: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
        }
        return t;
}

void task_free(task* const t) {
        free(t);
}

TASK_INT task_get_id(task* const t) {
        return t->id;
}
TASK_INT task_get_period(task* const t) {
        return t->period;
}
TASK_INT task_get_reldead(task* const t) {
        return t->reldead;
}
TASK_INT task_get_comp(task* const t, int i) {
        return t->comp[i];
}
float task_get_prob(task* const t, int i) {
        return t->prob[i];
}
float task_get_beta(task* const t) {
        return t->beta;
}

void task_set_id(task* t, TASK_INT id) {
        t->id = id;
}
void task_set_period(task* t, TASK_INT period) {
        t->period = period;
}
void task_set_reldead(task* t, TASK_INT reldead) {
        t->reldead = reldead;
}
void task_set_comp(task* t, TASK_INT comp, int i) {
        t->comp[i] = comp;
}
void task_set_prob(task* t, float prob, int i) {
        t->prob[i] = prob;
}
void task_set_beta(task* t, float beta) {
        t->beta = beta;
}
