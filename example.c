#include "src/regexjit.h"

int main(int argc, char **argv)
{
    if(argc != 2)
        return 1;

    regjit_regex_t *reg = regjit_compile(argv[1], 0);
    if(reg == NULL)
        return 1;

    return 0;
}