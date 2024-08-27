#include <stdio.h>
#include <stdlib.h>

#include "./src/cli/cli.h"

int main(void)
{
    if (cli_init()) {
        fprintf(stderr,"failed init cli\n");
        return -1;
    }

    if (cli_start()) {
        fprintf(stderr,"cli crushed\n");
        return -2;
    }

    return EXIT_SUCCESS;
}
