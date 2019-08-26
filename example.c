#include <stdio.h>
#include "src/regexjit.h"

int main(int argc, char **argv)
{
	if(argc < 2)
		return 1;

	regjit_regex_t *reg = regjit_compile(argv[1], 0);
	if(reg == NULL)
		return 1;

	size_t count = regjit_match_count(reg);
	regjit_match_t matches[count];

	for(int i = 2; i < argc; i++)
	{
		printf("matching %s := ", argv[i]);
		if(!regjit_match(reg, matches, argv[i]))
		{
			puts("NULL");
			continue;
		}

		puts("[");
		for(int j = 0; j < count; j++)
			printf("\t%.*s,\n", matches[j].end - matches[j].start, matches[j].start);
		puts("];");
	}

	regjit_destroy(reg);
	return 0;
}