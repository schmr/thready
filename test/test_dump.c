/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>

#include "dump.h"


struct state {
	void** src;
	void** dst;
	int n;
};


int setup_state(void** state) {
	struct state* s = calloc(1, sizeof(struct state));
	int n = 10;
	s->n = n;
	s->src = calloc(n, sizeof(void*));
	s->dst = calloc(n, sizeof(void*));
	if (s->src && s->dst) {
		for (int i = 0; i < n; i++) {
			int* a = calloc(1, sizeof(int));
			int* b = calloc(1, sizeof(int));
			if (a && b) {
				*a = i;
				*b = 2*i;
				*(s->src + i) = a;
				*(s->dst + i) = b;
			} else {
				return 1;
			}
		}
	} else {
		return 1;
	}
	*state = s;
	return 0;
}

int teardown_state(void** state) {
	struct state* s = *state;
	for (int i = 0; i<s->n; i++) {
		free(*(s->src + i));
		free(*(s->dst + i));
	}
	free(s->src);
	free(s->dst);
	free(s);
	return 0;
}


static void test_dump_merge_valid(void** state) {
	struct state* s = *state;
	void** m = merge(s->src, s->dst, s->n, s->n);
	for (int i = 0; i < s->n * 2; i++) {
		assert_non_null(*(m + i));
		int* p = *(m + i);
		int x = *p;
		assert_in_range(x, 0, 2*s->n);
	}
	free(m);
}


static void test_dump_uniq_valid(void** state) {
	struct state* s = *state;
	void** m = merge(s->src, s->src, s->n, s->n);
	int len = s->n * 2;
	void** u = calloc(len, sizeof(void*));
	int lenuniq = uniq(m, u, len);
	assert_int_equal(lenuniq, s->n);
	free(m);
	free(u);
}


int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_dump_merge_valid,
						setup_state,
						teardown_state),
		cmocka_unit_test_setup_teardown(test_dump_uniq_valid,
						setup_state,
						teardown_state),
	};
        return cmocka_run_group_tests(tests, NULL, NULL);
}
