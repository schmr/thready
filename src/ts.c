/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "ts.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "dump.h"
#include "json.h"
#include "rnd.h"
#include "selist.h"
#include "stats.h"
#include "task.h"

struct ts {
        struct selist* l;
};

ts* ts_init() {
        ts* tsy = calloc(1, sizeof(ts));
        if (tsy) {
                return tsy;
        } else {
                fprintf(stderr, "error allocating memory for tasksystem: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
        }
}

void ts_free(ts* tsy) {
        struct selist** l = &(tsy->l);
        do {
                task* t = selist_pop(l);
                if (t) {
                        task_free(t);
                        t = NULL;
                }
        } while (!selist_empty(*l));
        selist_free(*l);
        free(tsy);
}

task* ts_get_by_pos(ts const* const tsy, int const pos) {
        struct selist* l = tsy->l;
        task* t = selist_get(l, pos);
        if (t) {
                return t;
        } else {
                fprintf(stderr, "tasksystem pos. error: task %d not found\n",
                        pos);
                exit(EXIT_FAILURE);
        }
}

task* ts_get_by_id(ts const* const tsy, TASK_INT const id) {
        struct selist* l = tsy->l;
        int n = selist_length(l);

        task* t;
        for (int k = 0; k < n; k++) {
                t = selist_get(l, k);
                if (t) {
                        if (task_get_id(t) == id) {
                                return t;
                        }
                }
        }
        fprintf(stderr, "tasksystem key error\n");
        exit(EXIT_FAILURE);
}

int ts_get_pos_by_id(ts const* const tsy, TASK_INT const taskid) {
        task* t;
        int k = 0;
        // Without the (redundant?) assignment to t gcc complaints about maybe
        // unitialized on -O2
        t = ts_get_by_pos(tsy, k);
        for (k = 0; k < ts_length(tsy); k++) {
                t = ts_get_by_pos(tsy, k);
                if (task_get_id(t) == taskid) {
                        break;
                }
        }
        if (taskid != task_get_id(t)) {
                fprintf(stderr, "task id not found\n");
                exit(EXIT_FAILURE);
        }
        return k;
}

int ts_length(ts const* const tsy) {
        struct selist* l = tsy->l;
        return selist_length(l);
}

static void selist_to_ts(ts* tsy, struct selist** l) {
        int i = 0;
        while (i < selist_length(*l)) {
                intmax_t* id = selist_get(*l, i++);
                intmax_t* period = selist_get(*l, i++);
                intmax_t* reldead = selist_get(*l, i++);

                task* t = task_init();
                task_set_period(t, *period);
                task_set_reldead(t, *reldead);
                task_set_id(t, *id);

                for (int j = 0; j < TASK_NUM_COMP; j++) {
                        task_set_comp(t, *(intmax_t*)selist_get(*l, i++), j);
                }

                for (int j = 0; j < TASK_NUM_PROB; j++) {
                        task_set_prob(t, *(float*)selist_get(*l, i++), j);
                }

                task_set_beta(t, *(float*)selist_get(*l, i++));

                selist_push(&(tsy->l), t);
        }
}

void ts_read_json(ts* tsy, FILE* stream) {
        struct selist* l = (void*)0;

        json_parser* parser = dump_read_json_init(&l);
        dump_read_json_parse(parser, stream);
        dump_read_json_free(parser);

        selist_to_ts(tsy, &l);

        while (!selist_empty(l)) {
                intmax_t* val = selist_pop(&l);
                free(val);
        }
        selist_free(l);
}
