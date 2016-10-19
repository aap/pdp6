#include <stdio.h>
#include <stdint.h>
#include "pdp6common.h"

int
main()
{
	word w;
	while(w = readw(stdin), w != ~0)
		printf("%012lo\n", w);
	return 0;
}
