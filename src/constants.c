#include "regexjit-internal.h"

regjit_charset_t regjit_charset_dot = {
	.inverted = true,
	.lookupInitialized = false,
	.whitelist = "\r\n",
	.ranges = NULL,
};

static const regjit_charset_range_t digitRange = {
	.min = '0',
	.max = '9',
	.next = NULL,
};
regjit_charset_t regjit_charset_digits = {
	.inverted = false,
	.lookupInitialized = false,
	.whitelist = NULL,
	.ranges = &digitRange,
};
regjit_charset_t regjit_charset_non_digits = {
	.inverted = true,
	.lookupInitialized = false,
	.whitelist = NULL,
	.ranges = &digitRange,
};

regjit_charset_t regjit_charset_whitespace = {
	.inverted = false,
	.lookupInitialized = false,
	.whitelist = "\f\n\r\t\v",
	.ranges = NULL,
};
regjit_charset_t regjit_charset_non_whitespace = {
	.inverted = true,
	.lookupInitialized = false,
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
regjit_charset_t regjit_charset_word = {
	.inverted = false,
	.lookupInitialized = false,
	.whitelist = "_",
	.ranges = &smallAlphaRange,
};
regjit_charset_t regjit_charset_non_word = {
	.inverted = true,
	.lookupInitialized = false,
	.whitelist = "_",
	.ranges = &smallAlphaRange,
};
