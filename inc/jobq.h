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
 * @file jobq.h
 * @author Robert Schmidt
 * @brief Defines interface to job queues.
 */

#pragma once
#include "job.h"

typedef struct jobq jobq;

/**
 * @brief Initialize job queue.
 */
jobq* jobq_init();

/**
 * @brief Insert a job in the queue.
 *
 * The priority is defined by the function which is used to return a value from
 * the job, like its deadline or arrival time.
 *
 * @param jq Handle to job queue
 * @param j Handle to job
 * @param func Function pointer
 */
void jobq_insert_by(jobq* const jq, job* const j, JOB_INT (*func)(job* const));

/**
 * @brief Fetch and remove element of highest priority.
 * Returns NULL if job queue is empty.
 *
 * @param jq Handle to job queue
 * @return Handle to removed job
 */
job* jobq_pop(jobq* const jq);

/**
 * @brief Fetch element of highest priority without removal from queue.
 * Returns NULL if job queue is empty.
 *
 * @param jq Handle to job queue
 * @return Handle to job
 */
job* jobq_peek(jobq* const jq);

/**
 * @brief Free allocated memory of job queue.
 */
void jobq_free(jobq* const jq);

/**
 * @brief Dump content of job queue as part of complete simulator state dump.
 *
 * Writes pointers to jobs in array @p dst and returns length of the array.
 *
 * @param jq Handle to job queue
 * @param dst Handle to destination array
 */
int jobq_dump(jobq const* const jq, void*** dst);
