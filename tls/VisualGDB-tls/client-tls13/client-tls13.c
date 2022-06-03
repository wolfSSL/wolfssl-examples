#include <stdio.h>

int main(int argc, char *argv[])
{
	char sz[] = "Hello, World!\n";	/* Hover mouse over "sz" while debugging to see its contents */
	printf("%s", sz);
	fflush(stdout); /* <============== Put a breakpoint here */
	return 0;
}