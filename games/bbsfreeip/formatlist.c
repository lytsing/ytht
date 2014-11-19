#include <stdio.h>
#include <string.h>

int
main()
{
	char buf[1024], *ptr;
	while (fgets(buf, sizeof (buf), stdin) > 0) {
		ptr = strtok(buf, " \t\r\n");
		if (!ptr)
			continue;
		printf("%s", ptr);
		while ((ptr = strtok(NULL, " \t\r\n")))
			printf("\t%s", ptr);
		printf("\n");
	}
	return 0;
}
