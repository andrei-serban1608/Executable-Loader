#include <stdio.h>

void main() {
	int i, j;
	for(i = 1; i <= 3; i++) {
		for(j = 1; j <= 17; j++) {
			if((j >= 4 - i && j <= 5 + i) || (j >= 13 - i && j <= 14 + i))
				printf("*");
			else
				printf(" ");
		}
		printf("\n");
	}
	for(i = 1; i <= 9; i++) {
		for(j = 1; j <= 17; j++)
			if(j >= i && j <= 18 - i)
				printf("*");
			else
				printf(" ");
		printf("\n");
	}
}
