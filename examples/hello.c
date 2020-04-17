#include <stdio.h>
#include <string.h>

char *str1 = "I love ";
char *str2 = "bananas\n";

int main() {
  int x = 1;
	int y = 2;
	printf("hello world %d\n", (x+y)*(x+y));
  char* newStr = strdup(str1);
	strcat(newStr, str2);
	printf("%s", newStr);

  return 0;
}

