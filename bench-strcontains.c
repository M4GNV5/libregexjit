#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

typedef bool (*strcontains_t)(const char *, char);

bool strcontains_strchr(const char *str, char needle)
{
	return strchr(str, needle) != NULL;
}

bool strcontains_stupid(const char *str, char needle)
{
	while(*str != 0)
	{
		if(*str == needle)
			return true;
		str++;
	}

	return false;
}

bool strcontains_custom(const char *str, char needle)
{
	uint64_t mask = needle;
	mask |= (mask << 8);
	mask |= (mask << 16);
	mask |= (mask << 32);

	uint64_t add_magic = 0x7f7f7f7f7f7f7f7f;
	uint64_t holes = 0x8080808080808080;

	while(*str)
	{
		uint64_t haystack = *(uint64_t *)str;
		if((((haystack ^ mask) + add_magic) & holes) != holes)
			return true;

		str += 8;
	}

	return false;
}

uint64_t getTimestamp()
{
	struct timeval now;
	gettimeofday(&now, NULL);

	return now.tv_sec * 1000 + now.tv_usec / 1000;
}

void bench_func_with(strcontains_t func, const char *buff, char needle,
	const char *funcName, const char *buffName)
{
	printf("10000000x %s(%s, %c): ", funcName, buffName, needle);
	fflush(stdout);

	uint64_t start = getTimestamp();

	for(uint64_t i = 0; i < 10000000; i++)
		func(buff, needle);

	uint64_t end = getTimestamp();
	printf("%llu ms\n", end - start);
}

void bench_func(strcontains_t func, const char *funcName)
{
	const char *shortStr = "xsprdcvw";

	static char charset[128] = {0};
	if(charset[0] == 0)
	{
		for(int i = 0; i <= 127; i++)
			charset[i] = 127 - i;
	}



	bench_func_with(func, shortStr, 'x', funcName, "short");
	bench_func_with(func, shortStr, 'w', funcName, "short");
	bench_func_with(func, shortStr, 'z', funcName, "short");

	bench_func_with(func, charset, ' ', funcName, "charset");
	bench_func_with(func, charset, 'z', funcName, "charset");
}

int main()
{
	bench_func(strcontains_stupid, "stupid");
	bench_func(strcontains_strchr, "strchr");
	bench_func(strcontains_custom, "custom");
}
