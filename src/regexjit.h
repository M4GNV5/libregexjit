#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define REGJIT_FLAG_DEBUG 1

typedef struct regjit_regex regjit_regex_t;

typedef struct {
	const char *start;
	const char *end;
} regjit_match_t;

regjit_regex_t *regjit_compile(const char *expression, unsigned flags);
void regjit_destroy(regjit_regex_t *reg);

unsigned regjit_match_count(regjit_regex_t *reg);
bool regjit_match(regjit_regex_t *reg, regjit_match_t *matches, const char *text);
