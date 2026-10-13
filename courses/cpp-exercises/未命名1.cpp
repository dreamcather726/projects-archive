#include <stdio.h>
int global = 2023; // global variable
int main() {
	printf("input global number:");
	scanf("%d",&global);
    printf("%d\n", global);
    return 0;
}
