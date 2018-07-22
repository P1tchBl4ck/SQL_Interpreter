#ifndef __MY_ERROR_H
#define __MY_ERROR_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void my_error(char* func, int error){
	printf("ERROR on function %s\n", func);
	printf("%s\n", strerror(errno));
	exit(error);
}

void* my_dup(char* buffer, int* len){
	char* ret = (char*) malloc(sizeof(char) * (*len +1 ));
	for(int i = 0; i <= *len; i++)	*(ret+i) = *(buffer + i);
	return ret;
}

#endif //__MY_ERROR_H