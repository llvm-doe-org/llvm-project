#include <stdio.h>
#define N 1000


int main() {

   int a[N];
   int b[N];
   int i;

   for (i = 0; i < N; i++) {
      a[i] = 5;
      b[i] = 0;
   }

   #pragma acc enter data copyin(a[0:500],b)

   #pragma acc parallel present(a) copyout(b)
   { 
      b[10] = a[10];
   }

   printf("The value of b[10] should be 5, it is %d", b[10]);
        
   return 0;
}
