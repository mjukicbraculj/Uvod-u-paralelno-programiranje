#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <complex.h>
#include <string.h>
#include <math.h>
#include <time.h>

extern void dgemv_(char *trans, int *M, int *N, double *alpha, double *A, int *lda, double *x, int *incx, double *beta, double *y, int *incy);
extern double dnrm2_(int *N, double *X, int *incx);

double g(double x, double y)
{
    //double rez=sin(x);
    //printf("rez koji vracam za %d %d je %lg, ...\n", x, y, rez);
   // double rez=-sin(x)-sin(y);
    //double rez=x*x+y*y;
    double rez=sin(x)+y*y;
    return rez;
}

///s komandne linije ucitavamo ni, nj, h i epsilon
int main(int argc, char **argv)
{
    if(argc!=5)
    {
        fprintf(stderr, "greska: broj argumenata!\n");
        exit(EXIT_FAILURE);
    }

    MPI_Status status;
	MPI_Init(&argc, &argv);

    int size, rank, indexi[2];
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int Ni=atoi(argv[1]), Nj=atoi(argv[2]);
	double h=atof(argv[3]);
	double epsilon = atof(argv[4]);     ///epsilon ucitavamo s komandne linije

    if(rank==0)
    {
        ///svakom indexu posaljemo koji mu dio redaka pripada
        int koliko = Nj/size;
        int ostatak = Nj-size*koliko;
        int ukupno = koliko;
        indexi[0]=0;
        indexi[1]=koliko;
        int pomocni[2], dodatak;
        for(int i=1;i<size;++i)
		{
			///odredimo pocetak i kraj i  pošaljemo i-tom procesu
			if(size-ostatak<=i)dodatak = 1;
			else dodatak = 0;
			pomocni[0] = ukupno;
			pomocni[1] = ukupno+koliko+dodatak;
			//printf("%d procesu saljem %d, %d za pocetak i kraj\n", i, tmp_pocetak, tmp_kraj);
			MPI_Send(pomocni,2, MPI_INT, i, 0,MPI_COMM_WORLD);
			ukupno = pomocni[1];
		}
    }
    else
        MPI_Recv(indexi,2,MPI_INT,0,0,MPI_COMM_WORLD, &status);

    int m=(indexi[1]-indexi[0])*Ni;
    int n=Ni*Nj;
    /*double *matrica=malloc(m*n*sizeof(double));
    for(int i=0; i<m*n; ++i)
        matrica[i]=0;*/

    double *f=(double*)malloc(m*sizeof(double));
    double *eps=(double*)malloc(m*sizeof(double));
    for(int i=Ni*indexi[0]; i<Ni*indexi[1]; ++i)
    {
        int k=i-Ni*indexi[0];
        double x = ((i%Ni)+1);
        double y = (i/Ni+1);
        //f[k]=sin(x*h)+sin(y*h);
        //f[k]=-sin(x*h);
        //f[k]=4;
        f[k]=-sin(x*h)+2;
        f[k]=f[k]*h*h;
       // printf("%d je i..%lg %lg su x i y i %lg je f\n",i, x, y, f[i]);
        if(x==1&&y==1)f[k]-=(g((x-1)*h,y*h)+g(x*h,(y-1)*h));      ///sredjivanje rubnih uvjeta
        else if(x==Ni&&y==1)f[k]-=(g(x*h,(y-1)*h)+g((x+1)*h,y*h));
        else if(x==1&&y==Nj)f[k]-=(g((x-1)*h,y*h)+g(x*h,(y+1)*h));
        else if(x==Ni&&y==Nj)f[k]-=(g((x+1)*h,y*h)+g(x*h,(y+1)*h));
        else if(x==1)f[k]-=g((x-1)*h,y*h);
        else if(x==Ni)f[k]-=g((x+1)*h,y*h);
        else if(y==1)f[k]-=g(x*h,(y-1)*h);
        else if(y==Nj)f[k]-=g(x*h,(y+1)*h);
        else continue;
    }
   //for(int i=0; i<m; ++i)printf("f na %d mjestu je %lg u %d procesa\n", i, f[i], rank);
    /*MPI_File file;
    MPI_File_open(MPI_COMM_WORLD, "f.dat", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file );
    MPI_File_write_at(file, indexi[0]*Ni*sizeof(double), f, m, MPI_DOUBLE, &status);
    MPI_File_close(&file);*/
    /*for(int j = 0; j<m; ++j)
    {
        int i=j+indexi[0]*Ni;
        matrica[j*n+i]=-4;
        if(i%Ni==0)
        {
            if(i+1<Ni*Nj)matrica[j*n+i+1]=1;
        }
        else if(i%Ni==Ni-1)
        {
            if(i-1>=0)matrica[j*n+i-1]=1;
        }
        else
        {
            if(i+1<Ni*Nj)matrica[j*n+i+1]=1;
            if(i-1>=0)matrica[j*n+i-1]=1;
        }
        if(i+Ni<Ni*Nj)matrica[j*n+i+Ni]=1;
        if(i-Ni>=0)matrica[j*n+i-Ni]=1;
    }*/

    ///parni procesi su crveni, a neparni su crni
    ///prvo racunaju crveni procesi, nakon toga obavijeste susjeda o promjenama, i onda racunaju crni
    double *u=(double*)calloc(n,sizeof(double));
    int iter=0;
    double ukupno;
    //printf("TuSAAm\n");
    while(1)
    {
        if(rank%2==0)
        {
            for(int i=Ni*indexi[0]; i<Ni*indexi[1]; ++i)
            {
                double sum1=0;
                int k=i-Ni*indexi[0];
                if(i%Ni!=0)sum1+=u[i-1];
                if(i%Ni!=Ni-1)sum1+=u[i+1];
                if(i>=Ni)sum1+=u[i-Ni];
                if(i<Ni*(Nj-1))sum1+=u[i+Ni];
                u[i]=(f[k]-sum1)/(-4);
                //printf("%lg je vrijednost u na mjestu %d, broj iteracije je %d\n", u[i], i, iter);
            }

            ///saljemo susjedima
            if(rank-1>=0)
                MPI_Send(&u[Ni*indexi[0]],Ni, MPI_DOUBLE, rank-1, 2,MPI_COMM_WORLD);

            if(rank+1<size)
                MPI_Send(&u[Ni*(indexi[1]-1)],Ni, MPI_DOUBLE, rank+1, 2,MPI_COMM_WORLD);

        }

        if(rank%2==1)
        {
            //primi od susjeda
            if(rank-1>=0)
                MPI_Recv(&u[Ni*(indexi[0]-1)], Ni,MPI_DOUBLE,rank-1,2,MPI_COMM_WORLD, &status);
            if(rank+1<size)
                MPI_Recv(&u[Ni*indexi[1]], Ni,MPI_DOUBLE,rank+1,2,MPI_COMM_WORLD, &status);

            for(int i=Ni*indexi[0]; i<Ni*indexi[1]; ++i)
            {
                double sum1=0;
                int k=i-Ni*indexi[0];
                if(i%Ni!=0)sum1+=u[i-1];
                if(i%Ni!=Ni-1)sum1+=u[i+1];
                if(i>=Ni)sum1+=u[i-Ni];
                if(i<Ni*(Nj-1))sum1+=u[i+Ni];
                u[i]=(f[k]-sum1)/(-4);
                //printf("%lg je vrijednost u na mjestu %d, broj iteracije je %d\n", u[i], i, iter);
            }
            ///saljemo susjedima
             if(rank-1>=0)
                MPI_Send(&u[Ni*indexi[0]],Ni, MPI_DOUBLE, rank-1, 1,MPI_COMM_WORLD);

            if(rank+1<size)
                MPI_Send(&u[Ni*(indexi[1]-1)],Ni, MPI_DOUBLE, rank+1, 1,MPI_COMM_WORLD);

        }
        if(rank%2==0)
        {
                ///dobit ce nove izmjenjene indexe od susjeda koje koristi
                if(rank-1>=0)
                    MPI_Recv(&u[Ni*(indexi[0]-1)], Ni,MPI_DOUBLE,rank-1,1,MPI_COMM_WORLD, &status);

                if(rank+1<size)
                    MPI_Recv(&u[Ni*indexi[1]], Ni,MPI_DOUBLE,rank+1,1,MPI_COMM_WORLD, &status);
        }
        ///svaki proces izracuna svoj vektor greske

        for(int i=indexi[0]*Ni; i<indexi[1]*Ni; ++i)
        {
            int k=i-indexi[0]*Ni;
            eps[k]=f[k];
            double suma=0;
            if(i%Ni!=0)suma+=u[i-1];
            if(i%Ni!=Ni-1)suma+=u[i+1];
            if(i>=Ni)suma+=u[i-Ni];
            if(i<Ni*(Nj-1))suma+=u[i+Ni];
            suma-=4*u[i];
            eps[k]-=suma;
        }
        int inc=1;
        double norma = dnrm2_(&m, eps, &inc);

        MPI_Allreduce(&norma, &ukupno, 1, MPI_DOUBLE, MPI_SUM,MPI_COMM_WORLD);

        ++iter;
        if(ukupno<epsilon)break;
    }
    ///svaki proces ima tocno odredjene indexe koje zapisuje u datoteku

/*
    MPI_File_open(MPI_COMM_WORLD, "rjesenje.dat", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file );
    MPI_File_write_at(file, indexi[0]*Ni*sizeof(double), &u[indexi[0]*Ni], m, MPI_DOUBLE, &status);
    MPI_File_close(&file);
*/
    //if(rank==0)printf("UKUPNO odstupanje je %lg, a %d je iteracija, rank je %d\n", ukupno, iter, rank);
    ///svaki proces ce poslati 0-tom svoj dio u
    int recvcount=(int*)malloc(size*sizeof(int));
    int displ=(int*)malloc(size*sizeof(int));
    int pocetak=indexi[0]*Ni;
    MPI_Gather(&m, 1, MPI_INT, recvcount, 1, MPI_INT,0, MPI_COMM_WORLD);
    MPI_Gather(&pocetak, 1, MPI_INT, displ, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //for(int i=0; i<size; ++i) {printf("%d %d ",recvcount[i], displ[i]);}
    double polje[m];
    for(int i=0; i<m; ++i)polje[i]=u[i+pocetak];
   // for(int i=0; i<m; ++i)printf("%lg ", polje[i]);
   // printf("\n\n");
    MPI_Gatherv(polje, m, MPI_DOUBLE, u, recvcount, displ, MPI_DOUBLE, 0, MPI_COMM_WORLD );
    if(rank==0)
    {
        //for(int i=0; i<n;++i)printf("%lg ", u[i]);
        FILE *f;
        f=fopen("rezultat.txt", "w");
        for(int i=0; i<n; ++i)fprintf(f, "%d %.8lg\n", i, u[i]);
        fclose(f);
    }
    MPI_Finalize();
	return 0;
}
