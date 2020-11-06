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
 * @file eventloop.h
 * @author Robert Schmidt
 * @brief Defines interface to eventloop.
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "job.h"
#include "jobgen.h"

#ifndef EVL_INT
#define EVL_INT int64_t
#endif

typedef struct eventloop eventloop;

/**
 * @brief Possible results of running the event loop.
 */
typedef enum {
        EVL_OK = 0,
        EVL_DEADLINEMISS,
        EVL_PASS,
        EVL_OVERRUN
} eventloop_result;

/**
 * @brief Initialize the eventloop fetching the first job from the generator.
 *
 * If @p init is false, initialize the scheduler queue only.
 * This is required if the eventloop is restored from a state dump using @c
 * eventloop_read_json.
 *
 * @see eventloop_read_json
 * @param init If true, set simulation time to zero and fetch first job from
 * generator.
 * @return Handle to @c eventloop.
 */
eventloop* eventloop_init(jobgen* const jg, bool init);

/**
 * @brief Free memory of eventloop state.
 */
void eventloop_free(eventloop* evl);

/**
 * @brief Get current simulation time.
 */
EVL_INT eventloop_get_now(eventloop* evl);

/**
 * @brief Run eventloop until breaktime.
 *
 * Simulate Earliest Deadline First scheduling of arriving jobs until deadline
 * is missed or breaktime is reached.
 *
 * @startuml
 * start
 * while (now < breaktime) is (yes)
 *   partition CalcRuntime {
 *     :Get absolute arrival time of next job;
 *     if (arrival < breaktime) then (yes)
 *       :Set runtime to
 *       duration from now to
 *       next arrival;
 *     else (no)
 *       :Set runtime to
 *       duration from now to
 *       requested breaktime;
 *     endif
 *     :Adjust runtime if requested
 *     to consider breaking on overrun;
 *   }
 *
 *   partition Progress {
 *     while (runtime > 0) is (yes)
 *         :Set current job to job
 *         with earliest deadline;
 *         note right
 *           Peek scheduler
 *           job queue
 *         end note
 *         if (Queue empty) then (yes)
 *           break
 *         else (no)
 *         endif
 *         :Get deadline and
 *         computation demand
 *         from current job;
 *         if (runtime*speed <= computation) then (yes)
 *           :Progress time
 *           by runtime;
 *           :Subtract runtime
 *           from current job;
 *           note left
 *             Spend
 *             complete
 *             runtime
 *             budget on
 *             single job
 *           end note
 *           :Set runtime=0;
 *         else (no)
 *           :Progress time
 *           by computation;
 *           :Subtract computation
 *           from runtime;
 *           note right
 *             Update
 *             remaining
 *             runtime
 *             budget and
 *             finish job
 *           end note
 *           :Remove finished job
 *           from scheduler queue;
 *           :Count finished job;
 *         endif
 *
 *         :Count event;
 *         note right
 *           Finishing, preempting a job,
 *           or missing its deadline is
 *           counted as an event
 *         end note
 *
 *         if (Time > deadline) then (yes)
 *           :Reverse time to missed deadline;
 *           :Return EVL_DEADLINEMISS;
 *           stop
 *         else (no)
 *         endif
 *
 *     endwhile (no)
 *   }
 *
 *   :Check if breaktime is reached
 *   or would be reached if unused
 *   runtime budget is considered;
 *   note left
 *     Empty scheduler queue
 *     can result in unused
 *     runtime budget
 *   end note
 *   if () then (yes)
 *     :Return EVL_OK;
 *     note left
 *       Stop prior
 *       to arrival
 *       as requested
 *     end note
 *     stop
 *   else (no)
 *   endif
 *   :Check for task overrun;
 *   if () then (yes)
 *     :Return EVL_OVERRUN;
 *     stop
 *   else (no)
 *   endif
 *
 *
 *
 *   partition Arrival {
 *     :Progress time to next job arrival;
 *     :Insert job in scheduler queue;
 *     :Get next job from job generator;
 *     :Count event;
 *     note right
 *       Arrival of job
 *       is counted
 *       as an event
 *     end note
 *   }
 *
 * endwhile (no)
 * :Return EVL_OK;
 * stop
 * @enduml
 * @param evl Initialized eventloop.
 * @param breaktime Absolute time where simulation ends in the error-free case.
 * @param speed Processor speed in unit of work per timestep.
 * @return Signal successful scheduling or deadline miss.
 */
eventloop_result eventloop_run(eventloop* evl,
                               JOB_INT breaktime,
                               JOB_INT speed,
                               bool overrunbreak);

/**
 * @brief Human readable state print of eventloop.
 */
void eventloop_print_result(eventloop const* const evl,
                            eventloop_result const result);

/**
 * @brief Dump state of eventloop to JSON file.
 */
void eventloop_dump(eventloop const* const evl, FILE* stream);

/**
 * @brief Replaces some generator and scheduler state read in from JSON.
 *
 * @p evl needs to be a eventloop initialized with @c init false.
 *
 * @see eventloop_init
 * @param evl Handle to eventloop.
 * @param stream File pointer to state dump in JSON format.
 * @return Updated eventloop with partialy restored state.
 */
void eventloop_read_json(eventloop* evl, FILE* stream);
