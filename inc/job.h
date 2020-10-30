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
 * @file job.h
 * @author Robert Schmidt
 * @brief Defines interface to jobs.
 *
 * @remark In the implemented sporadic task model, jobs are released by tasks.
 */

#pragma once
#include <stdint.h>

#ifndef JOB_INT
#define JOB_INT int64_t
#endif

typedef struct job job;

/**
 * @brief Create and initialize a new job.
 *
 * Each job can reference the task it is created from by @p taskid.
 * The other parameters relate to the sporadic task model,
 * where @starttime is the job arrival time at the scheduler,
 * @p deadline is the absolute deadline of the job,
 * and @p computation is the amout of time required to execute the job on the
 * uniprocessor.
 * In mixed criticality simulations @p overruntime is the absolute time when
 * the job requires more computation than expected (budget + one time step).
 *
 * @param taskid Id of the task
 * @param starttime Job arrival time
 * @param overruntime Job overrun time
 * @param deadline Absolute deadline
 * @param computation Time required to execute the job on the uniprocessor
 * @returns Handle to initialized job.
 */
job* job_init(JOB_INT const taskid,
              JOB_INT const starttime,
              JOB_INT const overruntime,
              JOB_INT const deadline,
              JOB_INT const computation);

/**
 * @brief Free memory associated with job handle @p j.
 */
void job_free(job* const j);

JOB_INT job_get_taskid(job* const j);
JOB_INT job_get_starttime(job* const j);
JOB_INT job_get_overruntime(job* const j);
JOB_INT job_get_deadline(job* const j);
JOB_INT job_get_computation(job* const j);

/**
 * @brief Update computation of job @p j.
 *
 * @remark Allows to update a preempted job.
 */
void job_set_computation(job* const j, JOB_INT computation);
