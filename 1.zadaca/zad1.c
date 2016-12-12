#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


typedef struct
{
    int index, pocetak, kraj, stupci;
    pthread_mutex_t lock;
    char *matrica;
}thr_struct;

void* thr_fun(void *arg)
{
    int index = ((thr_struct*)arg)->index;
    int pocetak = ((thr_struct*)arg)->pocetak;
    int kraj = ((thr_struct*)arg)->kraj;
    pthread_mutex_t lock = ((thr_struct*)arg)->lock;
    int stupci = ((thr_struct*)arg)->stupci;
    char* matrica = ((thr_struct*)arg)->matrica;

    for(int i=pocetak; i<kraj; ++i)
        if(matrica[i] == 'o')
        {
            pthread_mutex_lock(&lock);
            printf("(%d,%d)\n", i/stupci, i%stupci);
            pthread_mutex_unlock(&lock);
        }
    return NULL;
}

/*void ispis_matrice(char *matrica, int m, int n)
{
    for(int i=0; i<m; ++i)
    {
        for(int j=0; j<n; ++j)
            printf("%c ", matrica[i*n+j]);
        printf("\n");
    }
}*/

int main(int argc, char** argv)
{
    char c, *matrica;
    int k=0, M, N;
    pthread_mutex_t lock;
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "\n mutex init failed\n");
        exit(EXIT_FAILURE);
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
    int duljina = M*(N+1);
    if(broj_dretvi<=0 || M<=0 || N<=0)
    {
        fprintf(stderr, "svi parametri moraju biti pozitivni\n");
        exit(EXIT_FAILURE);
    }

    FILE *f;
    f = fopen(argv[4], "r");
    if( f == NULL )
    {
        fprintf(stderr, "greska pri otvaranju datoteke\n");
        exit(1);
    }
    if((matrica = (char*) malloc(M*N*sizeof(char)))==NULL)
    {
        fprintf(stderr, "memory allocation error: matrica\n");
		exit(EXIT_FAILURE);
    }
    for(int i=0; i<duljina; ++i)
    {
        fscanf(f, "%c", &c);
        if(c == 'o' || c == '.')
            matrica[k++]=c;
    }
    fclose(f);

    pthread_t *thr_idx;
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL) {
		fprintf(stderr, "%s: memory allocation error\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    int dijeljenje =M*N/broj_dretvi;
    int ukupno = 0, dodatak;
    int ostatak = M*N-broj_dretvi*dijeljenje;
    thr_struct *podaci;
	if((podaci = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct)))==NULL)
	{
        fprintf(stderr, "memory allocation error: podaci \n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < broj_dretvi; ++i)
	{
        if(broj_dretvi-ostatak<=i) dodatak = 1;
        else dodatak = 0;
        podaci[i].index = i;
        podaci[i].pocetak = ukupno;
        podaci[i].kraj = ukupno+dijeljenje+dodatak;
        podaci[i].lock = lock;
        ukupno = podaci[i].kraj;
        podaci[i].stupci = N;
        podaci[i].matrica = matrica;
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

    free(matrica);
    free(thr_idx);
    free(podaci);
    return 0;
}
