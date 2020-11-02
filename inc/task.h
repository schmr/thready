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
 * @file task.h
 * @author Robert Schmidt
 * @brief Defines interface to tasks.
 */

#pragma once
#include <stdint.h>

#ifndef TASK_INT
#define TASK_INT int64_t
#endif

#define TASK_NUM_COMP 6
#define TASK_NUM_PROB 2
#define TASK_NUM_PARAM 12

typedef struct task task;

/**
 * @brief Allocate memory for new task.
 *
 * Each task is described by a list of integers and floating point numbers:
 *
 * - Task ID
 * - Period
 * - Relative deadline
 * - Computation0
 * - Computation1
 * - Computation2
 * - Computation3
 * - Computation4
 * - Computation5
 * - Probability0
 * - Probability1
 * - Beta
 *
 * Probability0 describes the probability to uniformly draw the computation
 * demand between Computation0 and Computation1. Probability1 describes the
 * probability to uniformly draw the computation demand between Computation2 and
 * Computation3. The remaining probability is to draw the computation demand
 * uniformly between Computation4 and Computation5. The parameter Beta is the
 * parameter of the exponential distributed inter arrival time between jobs.
 *
 * The following example shows a task system of three tasks,
 * where task 0 has a period and relative deadline of 5,
 * and computation is always uniformly drawn between 1 and 4.
 * The jobs of task 0 rise with the period,
 * or to be precise,
 * the probability of an arrival later than the minimum period is incredible low
 * (for task 0 the probability would be 1.384e-87).
 *
 * Task 1 has a period and relative deadline of 20,
 * and the computation demand is 1 with a probability of 0.9,
 * or between 2 and 4 with a probability of 0.09,
 * or between 5 and 8 with a probability of 0.01.
 * The jobs of task 1 rise on average 0.25 period after the minimum period.
 *
 *     [
 *             [0,5,5, 1,4, 0,0,  0,0, 1.0,0.0, 1000.0],
 *             [1,20,20, 1,1, 2,4,  5,8, 0.9,0.09, 4.0],
 *             [2,20,20, 1,2, 3,4,  5,8, 0.9,0.09, 4.0]
 *     ]
 *
 * @return Handle to new task
 */
task* task_init();
void task_free(task* const t);

TASK_INT task_get_id(task* const t);
TASK_INT task_get_period(task* const t);
TASK_INT task_get_reldead(task* const t);
TASK_INT task_get_comp(task* const t, int i);
float task_get_prob(task* const t, int i);
float task_get_beta(task* const t);

void task_set_id(task* t, TASK_INT id);
void task_set_period(task* t, TASK_INT period);
void task_set_reldead(task* t, TASK_INT reldead);
void task_set_comp(task* t, TASK_INT comp, int i);
void task_set_prob(task* t, float prob, int i);
void task_set_beta(task* t, float beta);
