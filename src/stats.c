/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "stats.h"

float uniformf(rnd_pcg_t** pcg, float min, float max) {
        float r = rnd_pcg_nextf(*pcg);
        return r * (max - min) + min;
}

float exponential(rnd_pcg_t** pcg, float beta) {
        float x = uniformf(pcg, 0.0f, 1.0f);
        return -logf(1.0f - x) * beta;
}
