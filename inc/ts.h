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
 * @file ts.h
 * @author Robert Schmidt
 * @brief Defines interface to task systems.
 */

#pragma once
#include <stdio.h>
#include "task.h"

typedef struct ts ts;

/**
 * @brief Allocate memory for task system.
 *
 * @return Handle to new task system
 */
ts* ts_init();
void ts_free(ts* tsy);

task* ts_get_by_pos(ts const* const tsy, int const pos);
task* ts_get_by_id(ts const* const tsy, TASK_INT const id);
int ts_get_pos_by_id(ts const* const tsy, TASK_INT const taskid);
int ts_length(ts const* const tsy);

/**
 * @brief Read task system from JSON stored in file.
 *
 * Each task is described by a list of integers and floating point numbers:
 * [_taskid_, _period_, _relative deadline_,
 * _computation0_,
 * _computation1_,
 * _computation2_,
 * _computation3_,
 * _computation4_,
 * _computation5_,
 * _probability0_,
 * _probability1_,
 * _beta_
 * ].
 *
 * The parameter _probability0_ describes the probability to uniformly draw the
 * computation demand between _computation0_ and _computation1_. The parameter
 * _probability1_ describes the probability to uniformly draw the computation
 * demand between _computation2_ and _computation3_. The remaining probability
 * is to draw the computation demand uniformly between _computation4_ and
 * _computation5_.
 *
 * The parameter _beta_ is the parameter of the exponential distributed
 * interarrival time between jobs.
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
 */
void ts_read_json(ts* tsy, FILE* stream);
