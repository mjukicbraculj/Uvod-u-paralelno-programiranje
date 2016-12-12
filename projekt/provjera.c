#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

int main()
{
    printf("ucitajte Ni i Nj i h!\n");
    int Ni, Nj, i, j,bzvz, pamtim_index;
    double h, tmp, max_raz=-1, raz , pamtim_vrijednost1, pamtim_vrijednost2;
    scanf("%d %d %lg", &Ni, &Nj, &h);
    double *polje=(double*)malloc(Ni*Nj*sizeof(double));
    FILE *f;
    f=fopen("rezultat.txt", "r");
    for(i=0; i<Ni*Nj; ++i)
        fscanf(f,"%d %lg",&bzvz, &polje[i]);
    for(i=0; i<Nj; ++i)
        for(j=0; j<Ni; ++j)     ///x je j, y je i
        {
            tmp=sin((j+1)*h)+(i+1)*h*(i+1)*h;
           // tmp=-sin((j+1)*h)-sin((i+1)*h);
            //if(i*Ni+j==0)printf("%lg\n", tmp);
            raz=fabs(tmp-polje[i*Ni+j]);
           // printf("%d je index, %lg je funkcijska %lg je iz datoteke\n", i*Ni+j, tmp, polje[i*Ni+j]);
            if(raz>max_raz)
            {
               // printf("tu sam %d\n", i*Ni+j);
                max_raz=raz;
                pamtim_index=i*Ni+j;
                pamtim_vrijednost1=tmp;
                pamtim_vrijednost2=polje[i*Ni+j];
            }
        }
    printf("maximalno odstupanje je %.8lg, na indexu %d, %.8lg je vrijednost funkcije, %.8lg je vrijednost aproksimacije\n", max_raz, pamtim_index, pamtim_vrijednost1, pamtim_vrijednost2);
    return 0;

}
