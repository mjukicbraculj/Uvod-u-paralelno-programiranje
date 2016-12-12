#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <complex.h>
#include <string.h>
#include <math.h>

///!!! -lm za cabs kada kompajliramo

extern void zaxpy_(int *N, double complex *alfa, complex *x, int *INCX, double complex *y, int *INCY );
extern double complex zdotc_(int *N, double complex *x, int *INCX, double complex *y, int *INCY);

int main(int argc, char **argv)
{
	if(argc!=3)
	{
		fprintf(stderr, "greska: broj argumenata\n");
		exit(EXIT_FAILURE);
	}
	int size, rank,i, tmp_pocetak, tmp_kraj, dodatak;
	int duljina_vektora = atoi(argv[1]);
	char *ime_datoteke = argv[2];
	double complex *vektor;
	vektor =(double complex*) malloc(duljina_vektora*sizeof(double complex));
	MPI_Status status;
	MPI_Init(&argc, &argv);		///alokacija memorije potrebne mpi-u i inicijalizacija osnovnih mpi objekata

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int koliko = duljina_vektora/size;
	int ostatak = duljina_vektora-size*koliko;
	///0-ti proces ce ucitat vektor i poslat dijelove ostalima
	if(rank==0)
	{
		FILE *f;
		f = fopen(ime_datoteke, "rb");
		if(f==NULL)
		{
			printf("grska:otvaranje datoteke");
			exit(EXIT_FAILURE);
		}
		fread(vektor, sizeof(double complex), duljina_vektora, f);
		/*printf("vektor je:\n");
		for(i=0; i<duljina_vektora;++i)
			printf("%lg+%lgi ", creal(vektor[i]), cimag(vektor[i]));
		printf("\n");*/
		int ukupno = koliko; ///0-ti proces ostavlja za sebe dio vektora
		for(i=1;i<size;++i)
		{
			///odredimo pocetak i kraj i  pošaljemo i-tom procesu
			if(size-ostatak<=i)dodatak = 1;
			else dodatak = 0;
			tmp_pocetak = ukupno;
			tmp_kraj = ukupno+koliko+dodatak;
			//printf("%d procesu saljem %d, %d za pocetak i kraj\n", i, tmp_pocetak, tmp_kraj);
			MPI_Send(&vektor[tmp_pocetak],tmp_kraj-tmp_pocetak, MPI_C_DOUBLE_COMPLEX, i, 0,MPI_COMM_WORLD);
			ukupno = tmp_kraj;
		}
	}
	else
	{	///ostali procesi primaju svoj pocetak i kraj
		MPI_Recv(vektor,duljina_vektora,MPI_C_DOUBLE_COMPLEX,0,0,MPI_COMM_WORLD, &status);
		//printf("%d proces je dobio:\n", rank);
		if(size-ostatak<=rank)koliko++;
		/*for(i=0; i<koliko;++i)
			printf("%lg+%lgi ", creal(vektor[i]), cimag(vektor[i]));
		printf("\n");*/
	}
	///svaki proces ima vektor duljine koliko
	///treba pronaci max element, onaj s najvecom normom
	double max_element=0;
	int max_index=0;
	for(i=0; i<koliko;++i)
	{
		if(i==0 || cabs(vektor[i])>max_element)
		{
			max_index = i;
			max_element = cabs(vektor[i]);
		}
	}
	//printf("%d proces ima max element s normom %lg, indexa %d\n", rank, max_element, max_index);
	///treba napravit redukciju za maksimalan element
	int pot = 1;
	double complex najveci = vektor[max_index];
	double complex potencijalni_najveci;
	char msg[10];
	while(pot<size)
	{
		pot*=2;
		///ako je index dijeljiv s pot onda on prima poruku od svog susjeda o njegovom max elementu
		if((size-1-rank)%pot == 0)
		{
			///primi poruku ako ima od koga
			if(rank-pot/2>=0)
			{
				MPI_Recv(msg, 10, MPI_CHAR ,rank-pot/2, 1,MPI_COMM_WORLD, &status);
				///sada kada sam primio poruku onda moram izracunat koji je veci element, onaj kojeg sam dobio ili moj
				int gledamo;
				double real, imag;
				sscanf(msg, "%d %lg %lg", &gledamo,&real, &imag );
				if(gledamo == 0)continue;
				potencijalni_najveci = real+imag*I;
				if(cabs(potencijalni_najveci)>cabs(najveci))
					najveci = potencijalni_najveci;
				//printf("%d je dobio poruku od %d i najveci element je %lf+%lfi\n", rank, rank-pot/2, creal(najveci), cimag(najveci));
			}
		}
		if((size-1-rank)%pot==pot/2)  ///Ako nema elemenata onda ne treba slati nista
		{
			///posaljem poruku ako imam kome, ali uvijek imam kome i to size-1-rank-pot/2
			//printf("%d salje poruku %d u redukciji za max element\n", rank, rank+pot/2);
			sprintf(msg, "%d %lg %lg", koliko, creal(najveci), cimag(najveci));
			MPI_Send(msg,strlen(msg)+1, MPI_CHAR, rank+pot/2, 1, MPI_COMM_WORLD);
		}

	}
	///vracamo se u natrag i obavijestimo cvorove onim redom kojim smo ih spajali
	//if(rank==0)printf("nakon redukcija potencija je %d\n",pot);
	while(pot>1)
	{
		if((size-1-rank)%pot==0)///ako je visekratnik potencije onda mora poslati poruku svom djetetu s kojim se spojio
		{
			if(rank-pot/2>=0)
			{
				//printf("%d salje poruku %d o najvecem elementu\n", rank, rank-pot/2);
				MPI_Send(&najveci, 1, MPI_C_DOUBLE_COMPLEX, rank-pot/2, 2, MPI_COMM_WORLD);
			}
		}
		if((size-1-rank)%pot==pot/2)///mora primiti poruku od roditelja
		{
			MPI_Recv(&najveci, 1, MPI_C_DOUBLE_COMPLEX, rank+pot/2, 2, MPI_COMM_WORLD, &status);
			//printf("%d je primio poruku o najvecem elementu od roditelja %d\n", rank, rank+pot/2);
		}
		pot/=2;
	}
	//if(rank == 0)printf("na kraju svega potencija je %d\n", pot);
	//printf("%d: NA KRAJU JE MOJ NAJVECI ELEMENT %lg+%lgi\n", rank,creal(najveci), cimag(najveci));
	///svaka dretva mora sve svoje elemente pdijeliti s najvecim elementom
	double suma=0;
	//printf("dijelim elemente\n");
	/*for(i=0;i<koliko;++i)
	{
		vektor[i]=vektor[i]/najveci;
		suma+=cabs(vektor[i])*cabs(vektor[i]);
	}*/
	double complex alfa = 1/najveci;
	int incr=1;
	double complex *pomocni_vektor;
	pomocni_vektor=(double complex*)calloc(koliko, sizeof(double complex));
	zaxpy_(&koliko, &alfa, vektor, &incr, pomocni_vektor, &incr);
    /*for(i=0; i<koliko;++i)
        printf("%lg+%lgi\n", creal(pomocni_vektor[i]), cimag(pomocni_vektor[i]));*/
	///sada normu
	suma = zdotc_(&koliko, pomocni_vektor, &incr, pomocni_vektor, &incr);
	//printf("Suma Je %lg a rank je %d\n", suma, rank);
	double privremena_suma;
	while(pot<size)
	{
		pot*=2;
		///ako je index dijeljiv s pot onda on prima poruku od svog susjeda o njegovom max elementu
		///ako je index nije dijeljiv onda on salje poruku
		if((size-1-rank)%pot == 0)
		{
			///primi poruku ako ima od koga
			if(rank-pot/2>=0)
			{
				MPI_Recv(&privremena_suma, 1, MPI_DOUBLE ,rank-pot/2, 3,MPI_COMM_WORLD, &status);
				//printf("%d je dobio poruku od %d i SUMA je %lf\n", rank, rank+pot/2, suma);
				suma+=privremena_suma;
			}
		}
		if((size-1-rank)%pot==pot/2)  ///Ako nema elemenata onda ne treba slati nista
		{
			///posaljem poruku ako imam kome, ali uvijek imam kome i to size-1-rank-pot/2
			//printf("%d salje poruku %d u redukciji za SUMU\n", rank, rank+pot/2);
			MPI_Send(&suma,1, MPI_DOUBLE, rank+pot/2, 3, MPI_COMM_WORLD);
		}
	}
	if(rank == size-1)
	{
		//printf("PRIJE KORJENOVANJA : %lg", suma);
		suma=sqrt(suma)*cabs(najveci);
		printf("%lg\n", suma);
	}
	MPI_Finalize();
	return 0;
}
