/*
thready - A lightweight and fast scheduling simulator
Written in 2019 by Robert Schmidt <rschmidt@uni-bremen.de>
To the extent possible under law, the author(s) have dedicated all copyright and
related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with
this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "dump.h"
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "selist.h"

void** merge(void** a, void** b, int na, int nb) {
        void** r = calloc(na + nb, sizeof(void*));
        if (r) {
                memcpy(r, a, na * sizeof(void*));
                memcpy(r + na, b, nb * sizeof(void*));
        }
        return r;
}

static int compare_ptr_descending(const void* a, const void* b) {
        uintptr_t* x = (uintptr_t*)a;
        uintptr_t* y = (uintptr_t*)b;
        if (*x < *y) {
                return 1;  // Return -1 if you want ascending, 1 if you want
                           // descending order
        } else if (*x > *y) { // GCOVR_EXCL_START
                return -1;  // Return 1 if you want ascending, -1 if you want
                            // descending order
        } // GCOVR_EXCL_STOP
        return 0;
}

int uniq(void* const* const src, void** dst, int n) {
        void** dup = calloc(n, sizeof(void*));
        if (dup) {
                memcpy(dup, src, n * sizeof(void*));
        } else { // GCOVR_EXCL_START
                return 0;
        } // GCOVR_EXCL_STOP
        qsort(dup, n, sizeof(void*), compare_ptr_descending);

        void* last = (void*)0;
        int k = 0;
        for (int i = 0; i < n; i++) {
                void* next = *(dup + i);
                if (next != last) {
                        *(dst + k) = next;
                        last = next;
                        k++;
                }
        }
        free(dup);
        return k;
}

static int dump_json_tostream_cb(void* userdata,
                                 const char* s,
                                 uint32_t length) {
        FILE* stream = userdata;
        if (fwrite(s, sizeof(char), length, stream)) {
                return 0;
        } else { // GCOVR_EXCL_START
                return 1;
        } // GCOVR_EXCL_STOP
}

json_printer* dump_json_tostream_init(FILE* stream) {
        json_printer* print = calloc(1, sizeof(json_printer));
        if (print) {
                if (json_print_init(print, dump_json_tostream_cb,
                                    stream)) {  // GCOVR_EXCL_START
                        fprintf(stderr, "error init dump json\n");
                        exit(EXIT_FAILURE);
                }  // GCOVR_EXCL_STOP
        }
        return print;
}

void dump_json_tostream_free(json_printer* p) {
        json_print_free(p);
        free(p);
}

void dump_json_tostream(json_printer* p, int64_t const data) {
        char s[DUMP_BUFLEN] = "";
        int slen = snprintf(s, DUMP_BUFLEN, "%" PRId64 "", data);
        json_print_raw(p, JSON_INT, s, slen);
}

static int callback_append(void* userdata,
                           int type,
                           const char* data,
                           __attribute__((unused)) uint32_t length) {
        struct selist** l = userdata;
        intmax_t* vali;
        float* valf;
        switch (type) {
                case JSON_FLOAT:
                        valf = calloc(1, sizeof(float));
                        if (valf) {
                                // See `man strtol` example for error handling
                                errno = 0;
                                *valf = strtof(data, NULL);
                                if ((errno == ERANGE &&  // GCOVR_EXCL_START
                                     (*valf == FLT_MAX || *valf == FLT_MIN)) ||
                                    (errno != 0 &&
                                     valf == 0)) {
                                        fprintf(
                                            stderr,
                                            "error converting string to int\n");
                                        exit(EXIT_FAILURE);
                                }  // GCOVR_EXCL_STOP
                                selist_push(l, valf);
                        } else {  // GCOVR_EXCL_START
                                fprintf(stderr, "error adding float value\n");
                                exit(EXIT_FAILURE);
                        }  // GCOVR_EXCL_STOP
                        break;
                case JSON_INT:
                        vali = calloc(1, sizeof(intmax_t));
                        if (vali) {
                                *vali = strtoimax(data, NULL, 10);
                                if (errno == ERANGE) {  // GCOVR_EXCL_START
                                        fprintf(
                                            stderr,
                                            "error converting string to int\n");
                                        exit(EXIT_FAILURE);
                                }  // GCOVR_EXCL_STOP
                                selist_push(l, vali);
                        } else {  // GCOVR_EXCL_START
                                fprintf(stderr, "error adding integer value\n");
                                exit(EXIT_FAILURE);
                        }  // GCOVR_EXCL_STOP
                        break;
        }
        return 0;
}

/* Fetch all numbers from JSON, ignore everything else */
json_parser* dump_read_json_init(void* cb_data) {
        json_parser* parser = calloc(1, sizeof(json_parser));
        json_config cfg = {0};
        cfg.allow_yaml_comments = 1;
        if (json_parser_init(parser, &cfg, callback_append,
                             cb_data)) {  // GCOVR_EXCL_START
                fprintf(stderr, "json parser init fail\n");
                exit(EXIT_FAILURE);
        }  // GCOVR_EXCL_STOP
        return parser;
}

void dump_read_json_parse(json_parser* p, FILE* stream) {
        char block[1024];
        int len;
        while ((len = fread(&block, sizeof(char), 1024, stream)) > 0) {
                uint32_t processed = 0;
                int ret = json_parser_string(p, block, len, &processed);
                if (ret) {  // GCOVR_EXCL_START
                        fprintf(stderr, "error parsing json at char %d\n",
                                processed);
                        exit(EXIT_FAILURE);
                }  // GCOVR_EXCL_STOP
        }
}

void dump_read_json_free(json_parser* p) {
        json_parser_free(p);
        free(p);
}
