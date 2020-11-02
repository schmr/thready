/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/**
 * @file main.c
 * @author Robert Schmidt
 * @brief Signal handling, command line parsing, and eventloop instrumentation.
 */

#include <assert.h>
#include <signal.h>  // TODO: switch to sigaction for portability
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eventloop.h"
#include "job.h"
#include "parg.h"

#define STATE_PREFIXBUFLEN 128
#define FILENAMEMAXLEN 255
#define NUM(a) (sizeof(a) / sizeof(*a))

struct state {
        ts* tsy;
        //struct jobgen_parameters* p;
        jobgen* jg;
        eventloop* evl;
        FILE* tasksystem;
        FILE* resume;
        int randomseed_jobtrace;
        char prefix[STATE_PREFIXBUFLEN];
        JOB_INT breaktime;
        JOB_INT speed;
        bool overrunbreak;
};

static struct state* state_reference;

static void catch_signals(__attribute__((unused)) int signo) {
        char fname[FILENAMEMAXLEN] = {0};
        strncpy(fname, state_reference->prefix, STATE_PREFIXBUFLEN);
        strcat(fname, "_signal_dump.json");
        FILE* stream = fopen(fname, "w");
        if (stream) {
                eventloop_dump(state_reference->evl, stream);
                fclose(stream);
        } else {
                fprintf(stderr, "state dump io error\n");
                exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
}

static void atexit_cleanup(void) {
        eventloop_free(state_reference->evl);
        jobgen_free(state_reference->jg);
        ts_free(state_reference->tsy);
        free(state_reference->p);
        free(state_reference);
        state_reference = (void*)0;
}

int main(int argc, char* argv[]) {
        struct state* s = calloc(1, sizeof(struct state));
        if (!s) {
                fprintf(stderr, "error allocating memory\n");
                exit(EXIT_FAILURE);
        }
        state_reference = s;

        // Setting default values
        s->breaktime = 60000;
        s->speed = 1;

        int prefixlen = 0;

        s->tsy = ts_init();

        struct parg_state ps;
        int c;
        parg_init(&ps);
        // abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ
        //  x     x x   x   x x xx  x
        while ((c = parg_getopt(&ps, argc, argv, "bhz:t:vj:r:n:w:")) != -1) {
                switch (c) {
                        case 1:
                                printf("nonoption '%s'\n", ps.optarg);
                                break;
                        case 'h':
                                printf(
                                    "Usage: thready [-h] [-v] "
                                    "[-r <statedump.json>] "
                                    "[-z jobtracerandomseed] "
                                    "[-b] "
                                    "-n dumpprefix "
                                    "-t breaktime "
                                    "-w work/timestep "
                                    "-j <tasksystemfile.json>\n");
                                exit(EXIT_SUCCESS);
                                break;
                        case 'v':
                                printf("thready %s\n", VERSION);
                                return EXIT_SUCCESS;
                                break;
                        // Dump and resume
                        case 'j':  // reading task system from JSON
                                s->tasksystem = fopen(ps.optarg, "r");
                                break;
                        case 'r':  // resume from JSON state dump
                                s->resume = fopen(ps.optarg, "r");
                                break;
                        case 'n':
                                prefixlen = strlen(ps.optarg);
                                if (prefixlen <= STATE_PREFIXBUFLEN - 1) {
                                        strncpy(s->prefix, ps.optarg,
                                                STATE_PREFIXBUFLEN);
                                } else {
                                        fprintf(stderr,
                                                "prefix too long > %d\n",
                                                STATE_PREFIXBUFLEN);
                                        exit(EXIT_FAILURE);
                                }
                                break;
                        // Simulation control
                        case 'b':
                                s->overrunbreak = true;
                                break;
                        case 'z':
                                s->randomseed_jobtrace = atoi(ps.optarg);
                                break;
                        case 't':
                                s->breaktime = atoll(ps.optarg);
                                break;
                        case 'w': // Processor speed; work done per timestep
                                s->speed = atoll(ps.optarg);
                                break;
                        case '?':
                                if ((ps.optopt == 't') || (ps.optopt == 'j') ||
                                    (ps.optopt == 'z') || (ps.optopt == 'r') ||
                                    (ps.optopt == 'n') || (ps.optopt == 'w')) {
                                        printf(
                                            "option -%c requires an argument\n",
                                            ps.optopt);
                                } else {
                                        printf("unknown option -%c\n",
                                               ps.optopt);
                                }
                                exit(EXIT_FAILURE);
                                break;
                        default:
                                printf("error: unhandled option -%c\n", c);
                                exit(EXIT_FAILURE);
                                break;
                }
        }

        if (!prefixlen) {
                fprintf(stderr, "no dump prefix specified\n");
                exit(EXIT_FAILURE);
        }

        // Read tasksystem from JSON
        if (s->tasksystem) {
                ts_read_json(s->tsy, s->tasksystem);
                fclose(s->tasksystem);
        } else {
                fprintf(stderr, "no tasksystem json file specified\n");
                exit(EXIT_FAILURE);
        }
        if (s->resume) {
                // Do not refill jobgenerator with jobs starting at zero if
                // we resume from a state dump.
                // The random generator state is not restored from the state
                // dump!
                s->jg = jobgen_init(s->tsy, s->randomseed_jobtrace, false);
        } else {
                s->jg = jobgen_init(s->tsy, s->randomseed_jobtrace, true);
        }
        if (s->resume) {
                s->evl = eventloop_init(s->jg, false);
                eventloop_read_json(s->evl, s->resume);
        } else {
                s->evl = eventloop_init(s->jg, true);
        }

        // Install handlers to free memory on exit and to state dump on signals
        if (atexit(atexit_cleanup)) {
                fprintf(stderr, "error registering atexit handler\n");
                exit(EXIT_FAILURE);
        }
        if (signal(SIGTERM, catch_signals) == SIG_ERR) {
                fprintf(stderr, "Error setting signal handler\n");
                exit(EXIT_FAILURE);
        }
        if (signal(SIGINT, catch_signals) == SIG_ERR) {
                fprintf(stderr, "Error setting signal handler\n");
                exit(EXIT_FAILURE);
        }
        if (signal(SIGTERM, catch_signals) == SIG_ERR) {
                fprintf(stderr, "Error setting signal handler\n");
                exit(EXIT_FAILURE);
        }

        eventloop_result r;
        if (s->breaktime <= eventloop_get_now(s->evl)) {
                // Nothing to simulate
                r = EVL_PASS;
        } else {
                r = eventloop_run(s->evl, s->breaktime, s->speed, s->overrunbreak);
        }

        // Dump results
        char fname[FILENAMEMAXLEN] = {0};
        strncpy(fname, s->prefix, STATE_PREFIXBUFLEN);
        strcat(fname, "_dump.json");
        FILE* stream = fopen(fname, "w");
        if (stream) {
                eventloop_dump(s->evl, stream);
                fclose(stream);
        } else {
                fprintf(stderr, "state dump io error\n");
                exit(EXIT_FAILURE);
        }

        eventloop_print_result(s->evl, r);
        exit(EXIT_SUCCESS);
}
