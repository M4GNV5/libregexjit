# RegexJIT Library

This library compiles regular expressions during runtime just in time (JIT).
This allows them run faster than with regular libraries while still allowing
dynamic regular expression in contrast to compile time compiled regular
expressions.

## Usage

```C
regjit_regex_t *regjit_compile(const char *expression, unsigned flags);
void regjit_destroy(regjit_regex_t *reg);

#define REGJIT_FLAG_DEBUG // dumps jit'ed function IR and machine code
#define REGJIT_FLAG_MULTILINE
#define REGJIT_FLAG_OPTIMIZE_0
#define REGJIT_FLAG_OPTIMIZE_1
#define REGJIT_FLAG_OPTIMIZE_2
#define REGJIT_FLAG_OPTIMIZE_3 // default
```
compile and destroy regular expressions. `flags` can be any of the constants or'ed
together. Only one optimization level at a time is supported.

```C
unsigned regjit_match_count(regjit_regex_t *reg);
bool regjit_match(regjit_regex_t *reg, regjit_match_t *matches, const char *text);
```
Match a regular expression to a text. `matches` should either be NULL, or an array
which can hold at least `rejit_match_count(reg)` elements.
The function returns `true` when the regular expression successfully matched or
`false` when not. In the latter case the contents of `matches` are invalid.

## Example

```C
void foo()
{
	regjit_regex_t *reg = regjit_compile("http://(.+)", 0);
	if(reg == NULL)
		return;

	size_t count = regjit_match_count(reg);
	regjit_match_t matches[count];

	if(!regjit_match(reg, matches, "the url is http://m4gnus.de, as you should know"))
		return;

	size_t len = matches[1].end - matches[1].start;
	printf("found url: %.*s\n", len, matches[1].start);
}
```

## Regex Flavour
The Regex Syntax is the same as the one used in Javascript, check
[MDN](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_Expressions/Cheatsheet)
for a documentation.

List of (not yet) implemented features:
- [x] `^`, `$` and the multiline flag
- [x] `\b` and `\B` word border
- [ ] `.*?` non-greedy match
- [ ] `x(?=y)` x followed by y
- [ ] `x(?!y)` x not followed by y
- [ ] `(?<=y)x` x preceeded by y
