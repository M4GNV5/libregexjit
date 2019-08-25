#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct regjit_regex regjit_regex_t;

typedef struct {
    size_t pos;
    size_t len;
} regjit_match_t;

typedef struct {
    regjit_match_t *matches;
    size_t count;
} regjit_result_t;

regjit_regex_t *regjit_compile(const char *expression, unsigned flags);

bool regjit_test(regjit_regex_t *reg, const char *text);
regjit_match_t *regjit_match(regjit_regex_t *reg, const char *text);

void regjit_destroy_match(regjit_result_t *result);
void regjit_destroy(regjit_regex_t *reg);
