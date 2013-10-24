#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>



int main()
{
FILE *f1,*f2,*f3;
int risp=0;
int v=1;
f1= fopen("prova.txt" , "w+");
f2= fopen("prova2.txt" , "w+");
printf("%d\n" , f1);
printf("%d\n" , f2);
do
{
 printf("Per uscire da qua premere 1\n");
 scanf("%d",&risp);
} while ( risp != 1 );

f3 = fopen("prova3.txt" , "w+");
fprintf(f3, "sono sempre in attesa, il problema è che mi avete strappato stdout");
fflush(stdin);
fflush(f3);
sleep(10);

}
