#include "regexjit-internal.h"

//no char whitelist and char ranges mean all chars match
const regjit_charset_t regjit_charset_all = {
	.inverted = false,
	.whitelist = NULL,
	.ranges = NULL,
};

static const regjit_charset_range_t digitRange = {
	.min = '0',
	.max = '9',
	.next = NULL,
};
const regjit_charset_t regjit_charset_digits = {
	.inverted = false,
	.whitelist = NULL,
	.ranges = &digitRange,
};
const regjit_charset_t regjit_charset_non_digits = {
	.inverted = true,
	.whitelist = NULL,
	.ranges = &digitRange,
};

const regjit_charset_t regjit_charset_whitespace = {
	.inverted = false,
	.whitelist = "\f\n\r\t\v",
	.ranges = NULL,
};
const regjit_charset_t regjit_charset_non_whitespace = {
	.inverted = true,
	.whitelist = "\f\n\r\t\v",
	.ranges = NULL,
};

static const regjit_charset_range_t bigAlphaRange = {
	.min = 'A',
	.max = 'Z',
	.next = &digitRange,
};
static const regjit_charset_range_t smallAlphaRange = {
	.min = 'a',
	.max = 'z',
	.next = &bigAlphaRange,
};
const regjit_charset_t regjit_charset_word = {
	.inverted = false,
	.whitelist = "_",
	.ranges = &smallAlphaRange,
};
const regjit_charset_t regjit_charset_non_word = {
	.inverted = true,
	.whitelist = "_",
	.ranges = &smallAlphaRange,
};
