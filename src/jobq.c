/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "jobq.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pqueue.h"

struct jobq {
        pqueue_t* pq;
};

typedef struct node_t {
        pqueue_pri_t pri;
        size_t pos;
        job* job;
} node_t;

static int cmp_pri(pqueue_pri_t next, pqueue_pri_t curr) {
        return (next > curr);
}

static pqueue_pri_t get_pri(void* a) {
        return ((node_t*)a)->pri;
}

static void set_pri(void* a, pqueue_pri_t pri) {
        ((node_t*)a)->pri = pri;
}

static size_t get_pos(void* a) {
        return ((node_t*)a)->pos;
}

static void set_pos(void* a, size_t pos) {
        ((node_t*)a)->pos = pos;
}

jobq* jobq_init() {
        jobq* jq = calloc(1, sizeof(jobq));
        if (!jq) {  // GCOVR_EXCL_START
                fprintf(stderr, "error allocating memory for jobq: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
        jq->pq = pqueue_init(10, cmp_pri, get_pri, set_pri, get_pos, set_pos);
        if (!jq->pq) { // GCOVR_EXCL_START
                fprintf(stderr, "error allocating memory for jobq\n");
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
        return jq;
}

void jobq_insert_by(jobq* const jq, job* const j, JOB_INT (*func)(job* const)) {
        JOB_INT pri = func(j);
        node_t* n = calloc(1, sizeof(node_t));
        if (!n) { // GCOVR_EXCL_START
                fprintf(stderr, "error allocating memory for jobq node: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
        n->pri = pri;
        n->job = j;
        if (pqueue_insert(jq->pq, n)) { // GCOVR_EXCL_START
                fprintf(stderr, "error inserting jobq node\n");
                exit(EXIT_FAILURE);
        } // GCOVR_EXCL_STOP
}

job* jobq_pop(jobq* const jq) {
        job* j = NULL;
        node_t* n = pqueue_pop(jq->pq);
        if (n) {
                j = n->job;
                free(n);
        }
        return j;
}

job* jobq_peek(jobq* const jq) {
        node_t* n = pqueue_peek(jq->pq);
        if (n) {
                return n->job;
        } else {
                return NULL;
        }
}

void jobq_free(jobq* const jq) {
        node_t* n = NULL;
        while ((n = pqueue_pop(jq->pq))) {
                job_free(n->job);
                free(n);
        }
        pqueue_free(jq->pq);
        jq->pq = (void*)0;
        free(jq);
}

static pqueue_t* pqueue_duplicate(pqueue_t const* const src) {
        pqueue_t* dst;

        dst = pqueue_init(src->size, src->cmppri, src->getpri, set_pri,
                          src->getpos, set_pos);
        dst->size = src->size;
        dst->avail = src->avail;
        dst->step = src->step;
        memcpy(dst->d, src->d, (src->size * sizeof(void*)));

        return dst;
}

int jobq_dump(jobq const* const jq, void*** dst) {
        pqueue_t* dup = pqueue_duplicate(jq->pq);
        *dst = calloc(pqueue_size(dup), sizeof(void*));
        int i = 0;
        if (*dst) {
                node_t* n;
                while ((n = pqueue_pop(dup))) {
                        *(*dst + i) = n->job;
                        i++;
                }
        }
        pqueue_free(dup);
        return i;
}
