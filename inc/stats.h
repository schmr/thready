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
 * @file stats.h
 * @author Robert Schmidt
 * @brief Draw numbers from random distributions
 */

#pragma once
#include <math.h>
#include <stdint.h>
#include "rnd.h"

/**
 * @brief Draw uniformly distributed random integer
 */
int uniform(rnd_pcg_t** pcg, int min, int max);

/**
 * @brief Draw uniformly distributed random float
 */
float uniformf(rnd_pcg_t** pcg, float min, float max);

/**
 * @brief Draw float from exponential distribution
 *
 * The probability density function is parameterized with @p beta:
 * @f[
 *      \frac{1}{\beta}\mathrm{e}^{-x/\beta} \quad x \geq 0
 * @f]
 */
float exponential(rnd_pcg_t** pcg, float beta);
