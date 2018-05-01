// test if size of function can be obtained

#include <stdio.h>
extern char etext, edata, end;

typedef int(* fp)(float, char);

void Sub(void *p) {
   int *y = (int *)p;

   printf("Hello, World!\n");
}

int main(int argc, char *argv[]) {
   int i, x, y, z;

   x = 3; y = 4; z = x + y;

   Sub((void *)&x);

   printf("size of fp is %d.\n", sizeof(fp));
   printf("size of &main is %d.\n", sizeof(&main)); // addr is OK
   printf("size of main is %d.\n", sizeof(main));   // possible?
   printf("size of void is %d.\n", sizeof(void));   // possible?
   printf("main starts at %x.\n", main);            // possible?

   printf("etext is %x (%d).\n", &etext, etext);
   printf("edata is %x (%d).\n", &edata, edata);
   printf("end is %x (%d).\n", &end, end);

   for(i=0; i<argc; i++) printf("%d -> %s\n", i, argv[i]);   // given args

   printf("%d -> %s\n", i, argv[i]); // null here

   while(argv[++i]) printf("%d -> %s\n", i, argv[i]); // continue? till null

   printf("%d -> %s\n", i, argv[i]); // null here

   return 0;
}
