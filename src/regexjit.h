#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define REGJIT_FLAG_DEBUG 0x01
#define REGJIT_FLAG_MULTILINE 0x02
#define REGJIT_FLAG_OPTIMIZE_0 0x12 // 0x04 | 0x08
#define REGJIT_FLAG_OPTIMIZE_1 0x04
#define REGJIT_FLAG_OPTIMIZE_2 0x08
#define REGJIT_FLAG_OPTIMIZE_3 0x00 // default
// flags > 0xff are internal and defined in regexjit-internal.h

typedef struct regjit_regex regjit_regex_t;

typedef struct {
	const char *start;
	const char *end;
} regjit_match_t;

regjit_regex_t *regjit_compile(const char *expression, unsigned flags);
void regjit_destroy(regjit_regex_t *reg);

unsigned regjit_match_count(regjit_regex_t *reg);
bool regjit_match(regjit_regex_t *reg, regjit_match_t *matches, const char *text);
