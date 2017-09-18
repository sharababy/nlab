#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	
	char *str,*a;

	str = "hell";
	a = "o!";

	
	size_t len = strlen(str) + strlen(a);
	char str2[len]; 
	strcpy(str2, str);
	strcat(str2, a);

	printf( "%s\n", str2 );

	return 0;

}
