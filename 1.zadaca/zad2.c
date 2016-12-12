#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>



extern double ddot_(int *N, double *X, int *INCRX, double *Y, int *INCRY);
extern void dgemv_(char *TRANS, int *M, int *N, double *ALPHA, double *A, int *LDA, double *X, int *INCX, double *BETA, double *Y, int *INCY);
extern void daxpy_(int *duljina, double *ALPHA, double *x, int *INCX, double *y, int *INCY);

int broj_dretvi, n, m, N;
double w;
double *A, *b, *x0, *delta,  *norma;
char *result;
FILE *f;
pthread_barrier_t barijera1;
typedef struct {
	int index;
	int start_n;
    int end_n;
    int start_m;
    int end_m;
    int retci, stupci;
    int broj_iteracija;
    pthread_mutex_t lock;
} thr_struct;


void scan(char *name, double *buffer, int size)
{
    FILE *f1;
    f1 = fopen(name, "rb");
    if(f1 == NULL)
    {
        fprintf(stderr, "greska pri otvaranju datoteke %s\n", name);
        exit(EXIT_FAILURE);
    }
    fread(buffer, sizeof(double), size, f1);
    fclose(f1);
}

void* thr_fun(void *arg)
{
    double suma;
    int index = ((thr_struct*)arg)->index;
    int start_n = ((thr_struct*)arg)->start_n;
    int end_n = ((thr_struct*)arg)->end_n;
    int start_m = ((thr_struct*)arg)->start_m;
    int end_m = ((thr_struct*)arg)->end_m;
    int retci = ((thr_struct*)arg)->retci;
    int stupci = ((thr_struct*)arg)->stupci;
    int broj_iteracija = ((thr_struct*)arg)->broj_iteracija;
    pthread_mutex_t lock = ((thr_struct*)arg)->lock;

    ///skalarni produkt od stupca matrice je norma^2
    for(int i=start_n; i<end_n; ++i)
        norma[i]=ddot_(&retci,  A+i, &stupci, A+i, &stupci);

    ///racunamo r
    char TRANS='t';
    double ALPHA=-1.0, BETA=1.0;
    int incr=1;
    int sub_m = end_m - start_m;
    int sub_n = end_n - start_n;
    dgemv_(&TRANS, &stupci, &sub_m, &ALPHA, A+start_m*stupci, &stupci, x0, &incr, &BETA, b+start_m, &incr);

    for(int k=0; k<broj_iteracija; ++k)
    {
        pthread_barrier_wait(&barijera1);
        for(int j=start_n; j<end_n; ++j)
            delta[j]=(ddot_(&retci, &A[j], &stupci, b, &incr)*w)/norma[j];   ///skalarni umnozak j-tog stupca matrice A i r

        pthread_barrier_wait(&barijera1);
        daxpy_(&sub_n, &BETA,delta+start_n, &incr, x0+start_n, &incr);
        dgemv_(&TRANS, &stupci, &sub_m, &ALPHA, A+n*start_m, &stupci,delta, &incr, &BETA, b+start_m, &incr);
    }
    for(int i=start_n; i<end_n; ++i)
    {   pthread_mutex_lock(&lock);
        fprintf(f, "%d %lg\n", i, x0[i]);
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}
int main(int argc, char** argv)
{

    if(argc != 10)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }
    pthread_mutex_t lock;
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    broj_dretvi=atoi(argv[1]);
    m=atoi(argv[2]);
    n=atoi(argv[3]);
    w=atof(argv[4]);
    N=atoi(argv[5]);
    result = argv[9];


    f = fopen(result, "w");
    if(m<=0 || n<=0 || w<=0 || broj_dretvi<=0 || N<=0)
    {
        fprintf(stderr, "Svi parametri moraju biti pozitivni\n");
        exit(EXIT_FAILURE);
    }

    ///barijera
     pthread_barrier_init(&barijera1, NULL, broj_dretvi);

    ///alociramo matricu i vektore
    if((A = (double*)malloc(n*m*sizeof(double)))==NULL)
    {
        fprintf(stderr, "memory allocation error: A\n");
		exit(EXIT_FAILURE);
    }
    if((b = (double*)malloc(m*sizeof(double)))==NULL)
    {
        fprintf(stderr, "memory allocation error: b\n");
		exit(EXIT_FAILURE);
    }
    if((x0 = (double*)malloc(n*sizeof(double)))==NULL)
    {
        fprintf(stderr, "memory allocation error: x0\n");
		exit(EXIT_FAILURE);
    }
    if((delta = (double*)malloc(n*sizeof(double)))==NULL)
    {
        fprintf(stderr, "memory allocation error: delta\n");
		exit(EXIT_FAILURE);
    }
    if((norma = (double*)malloc(n*sizeof(double)))==NULL)
    {
        fprintf(stderr, "memory allocation error: delta\n");
		exit(EXIT_FAILURE);
    }


    ///ucitamo matricu i vektore
    scan(argv[6], A, m*n);
    scan(argv[7], b, m);
    scan(argv[8], x0, n);


///podjela stupaca na dretve
    int dijeljenje1 = n/broj_dretvi;
    int ukupno1 = 0, dodatak1;
    int ostatak1 = n-broj_dretvi*dijeljenje1;
///podjela redaka na dretve
    int dijeljenje2 = m/broj_dretvi;
    int ukupno2 = 0, dodatak2;
    int ostatak2 = m-broj_dretvi*dijeljenje2;

    thr_struct* podaci = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));

    pthread_t *thr_idx;
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL) {
		fprintf(stderr, "%s: memory allocation error\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < broj_dretvi; ++i)
	{
        if(broj_dretvi-ostatak1<=i) dodatak1 = 1;
        else dodatak1 = 0;
        podaci[i].index = i;
        podaci[i].start_n = ukupno1;
        podaci[i].end_n = ukupno1+dijeljenje1+dodatak1;
        ukupno1 = podaci[i].end_n;
        if(broj_dretvi-ostatak2<=i) dodatak2 = 1;
        else dodatak2 = 0;
        podaci[i].start_m = ukupno2;
        podaci[i].end_m = ukupno2+dijeljenje2+dodatak2;
        podaci[i].stupci=n;
        podaci[i].retci=m;
        podaci[i].lock = lock;
        podaci[i].broj_iteracija=N;
        ukupno2 = podaci[i].end_m;
		if (pthread_create(&thr_idx[i], NULL, thr_fun,(void*)&podaci[i])) {
			fprintf( stderr,"%s: error creating thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
		}
    }
    for (int i = 0; i < broj_dretvi; ++i)
		if (pthread_join(thr_idx[i], NULL)) {
			fprintf(stderr, "%s: error joining thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
    }


    fclose(f);
    free(A);
    free(b);
    free(x0);
    free(delta);
    free(norma);
    free(thr_idx);
    free(podaci);

    if(pthread_barrier_destroy(&barijera1))
    {
    	fprintf(stderr, "%s: error destroying barrier\n", argv[0]);
    	exit(EXIT_FAILURE);
    }

    return 0;
}
