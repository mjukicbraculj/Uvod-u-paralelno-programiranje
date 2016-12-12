#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

int M, N;
pthread_mutex_t lock;
char *ime_dat;
typedef struct
{
    int index;
	int pocetak;
    int kraj;
}thr_struct;

void* thr_fun(void *arg)
{
    char znak;
    int index = ((thr_struct*)arg)->index;
    int pocetak = ((thr_struct*)arg)->pocetak;
    int kraj = ((thr_struct*)arg)->kraj;
    FILE *f;
    f = fopen(ime_dat, "r");
    printf("%d dretva ide na %d poziciju\n", index, pocetak);
    fseek(f, pocetak, SEEK_SET);
    for(int i=pocetak; i<kraj; ++i)
     {
        fscanf(f, "%c", &znak);
        if(znak == 'o')
        {
            pthread_mutex_lock(&lock);
            printf("Kamp je na (%d,%d) i dretva je %d\n", i/(N+1), i%(N+1), index);
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }


    if(argc != 5)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }
    int broj_dretvi;
    broj_dretvi = atoi(argv[1]);
    M = atoi(argv[2]);
    N = atoi(argv[3]);
    int duljina = M*N+M;
    ime_dat = argv[4];
    /*FILE *f;
    f = fopen(argv[4], "r");
    if( f == NULL )
    {
        printf("greska pri otvaranju datoteke\n");
        exit(1);
    }
    matrica = (char*) malloc(duljina*sizeof(char));
    for(int i=0; i<duljina; ++i)
        fscanf(f, "%c", &matrica[i]);
*/
    ///ispis matrice
    /*for(int i=0; i<M; ++i)
    {
        for(int j=0; j<N; ++j)
            printf("%c ", matrica[i*M+j]);
        printf("\n");
    }*/

    //fclose(f);
    /*trajanje -= clock();
    printf("Sekvencijalno ucitavanje traje  %f   sekundi\n", ((float)-trajanje)/CLOCKS_PER_SEC);
    trajanje = clock();**/
    clock_t trajanje;
    trajanje = clock();
    pthread_t *thr_idx;
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL) {
		fprintf(stderr, "%s: memory allocation error\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    int dijeljenje = (M*N+M)/broj_dretvi;
    int ukupno = 0, dodatak;
    int ostatak = (M*N+M)-broj_dretvi*dijeljenje;
	thr_struct* podaci = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));


	for (int i = 0; i < broj_dretvi; ++i)
	{
        if(broj_dretvi-ostatak<=i) dodatak = 1;
        else dodatak = 0;
        podaci[i].index = i;
        podaci[i].pocetak = ukupno;
        podaci[i].kraj = ukupno+dijeljenje+dodatak;
        ukupno = podaci[i].kraj;
		if (pthread_create(&thr_idx[i], NULL, thr_fun,(void*)&podaci[i]))
		{
			fprintf( stderr,"%s: error creating thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
		}

    }
    for (int i = 0; i < broj_dretvi; ++i)
		if (pthread_join(thr_idx[i], NULL))
		{
			fprintf(stderr, "%s: error joining thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
        }
    trajanje -= clock();
    printf("Trazenje kampova sa sekvencijalnim citanjem i %d dretvi treba   %f   sekundi\n", broj_dretvi, ((float)-trajanje)/CLOCKS_PER_SEC);

    return 0;
}

