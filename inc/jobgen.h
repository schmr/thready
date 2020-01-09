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
 * @file jobgen.h
 * @author Robert Schmidt
 * @brief Defines interface to the job generator.
 */

#pragma once
#include <stdbool.h>
#include "job.h"
#include "jobq.h"
#include "ts.h"

typedef struct jobgen jobgen;

/**
 * @brief Allocate and optionaly initialize memory for a job generator.
 *
 * A job generator creates the jobs which are released from tasks for
 * consumption by the scheduler (eventloop). It is possible to skip
 * initialization which is needed to resume from a state dump of a former
 * simulation. To simulate different random job traces the @p seed can be
 * changed.
 *
 * @param tasksystem System of tasks for which jobs should be generated
 * @param seed Random seed for the job generation
 * @param refill Boolean triggering initialization
 * @returns Handle to job generator
 */
jobgen* jobgen_init(ts const* const tasksystem, uint32_t seed, bool refill);

/**
 * @brief Free memory allocated for a job generator @p jg.
 */
void jobgen_free(jobgen* jg);

/**
 * @brief Restore tracked time for all tasks on state resume.
 */
void jobgen_set_simtime(jobgen* jg, JOB_INT* simtimes, int len);
ts const* jobgen_get_tasksystem(jobgen const* const jg);

/**
 * @brief Get next arriving job.
 *
 * Job arrival is handled by the job generator by sorting all task's jobs by
 * arrival time.
 *
 * @param jg Job generator handle
 * @returns Next arriving job
 */
job* jobgen_rise(jobgen* jg);

/**
 * @brief Dump the state of the job generator for possible future resume of
 * simulation.
 */
int jobgen_dump(jobgen const* const jg, void*** dst);

/**
 * @brief Replace the internal job priority queue to support resume from
 * simulation state dump.
 */
void jobgen_replace_jobq(jobgen* const jgen, jobq* const jq);

/**
 * @brief Create next batch of jobs.
 * @see jobgen_init
 */
void jobgen_refill_all(jobgen* jg);
