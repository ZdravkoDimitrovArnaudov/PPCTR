#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <omp.h>
#include <stdlib.h>

#define RAND rand() % 100

//#define DEBUG 
#define PARALLEL


void init_mat_inf ();
void init_mat_sup ();
void matmul ();
void matmul_sup ();
void matmul_inf ();
void print_mat (float *M, int dim); // TODO

/* Usage: ./matmul <dim> [block_size]*/


int main (int argc, char* argv[]) {
	
	//srand(clock());
	int block_size = 1, dim;
	float *A, *B, *C;
	
	//para medir el tiempo 
	double start;
	double end;
	
	
	dim = atoi (argv[1]); //dimensión de la matriz
	if (argc == 3) block_size = atoi (argv[2]); //podemos indicar un segundo argumento que sería 
	
	
	//reservamos memoria dinámica para las matrices
	A = (float*) malloc (dim*dim*sizeof(float));
	B = (float*) malloc (dim*dim*sizeof(float));
	C = (float*) malloc (dim*dim*sizeof(float));
	
   //inicializamos matriz diagonal superior A
   init_mat_sup(dim, A);
   
   
   //imprimimos matriz A (cambiar a función)
   #ifdef DEBUG
	   printf ("Matriz diagonal superior A\n");
	   print_mat(A,dim);
   #endif
	
	//inicializamos matriz diagonal superior B
   init_mat_sup(dim, B);
   
    //imprimimos matriz B (cambiar a función)
   #ifdef DEBUG
	   printf ("Matriz diagonal superior B\n");
	   print_mat(B,dim);
	   
	#endif
   
   //multiplicamos las matrices y lo guardamos en la matriz C.
   start = omp_get_wtime();
   matmul_sup(A,B,C,dim); //multiplica las matrices con la función correspondiente
   end=omp_get_wtime();
   
 //imprimimos matriz C(cambiar a función)
   #ifdef DEBUG
	   printf ("Matriz diagonal superior C\n");
	   print_mat(C,dim);
	#endif
   
   printf ("Tiempo de ejecución: %f segundos\n", end-start);
   printf ("\n");

	exit (0);
}



void print_mat (float *M, int dim){
	for (int i = 0; i < (dim*dim); i++){
		if ((i % dim) == 0){
		   printf ("\n");
	   }
	   printf (": %f", M[i]);
	   
   }
   printf ("\n");
   
}

void init_mat_inf (int dim, float *M)
{
	int i,j;
	//m,n,k;

	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			if (j >= i)
				M[i*dim+j] = 0.0;
			else
				M[i*dim+j] = RAND;
		}
	}
}

void init_mat_sup (int dim, float *M)
{
	int i,j;
	//m,n,k;

	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			if (j <= i)
				M[i*dim+j] = 0.0;
			else
				M[i*dim+j] = RAND;
		}
	}
}


void matmul (float *A, float *B, float *C, int dim) {
	int i, j, k;
	
	#ifdef PARALLEL //ejecución paralela
	int max_threads = omp_get_max_threads(); //maximos threads logicos
	
	#pragma omp parallel private(i,j,k) shared(A,B,C,dim) num_threads(max_threads)//paralllel area
	{ 
	
	#pragma omp for 
	for (i=0; i < dim; i++)
		for (j=0; j < dim; j++)
			C[i*dim+j] = 0.0;
			
			
		#pragma omp for 
		for (i=0; i < dim; i++)
			for (j=0; j < dim; j++)
				for (k=0; k < dim; k++)
					C[i*dim+j] += A[i*dim+k] * B[j+k*dim];
	
	}
	#else //ejecución sencuencial
	
	for (i=0; i < dim; i++)
		for (j=0; j < dim; j++)
			C[i*dim+j] = 0.0;
			
		for (i=0; i < dim; i++)
			for (j=0; j < dim; j++)
				for (k=0; k < dim; k++)
					C[i*dim+j] += A[i*dim+k] * B[j+k*dim];
	
	#endif
		
}

void matmul_sup (float *A, float *B, float *C, int dim)
{
	int i, j, k;
	
	//ejecución paralela
	#ifdef PARALLEL
	int max_threads = omp_get_max_threads(); //maximos threads logicos

	#pragma omp parallel private(i,j,k) shared(A,B,C,dim) num_threads(max_threads)  
	{
	
		#pragma omp for 
		for (i=0; i < dim; i++)
			for (j=0; j < dim; j++)
				C[i*dim+j] = 0.0;
				
		#pragma omp for schedule (static)
		for (i=0; i < dim -1; i++)
			for (j=0; j < dim; j++)
				for (k=(i+1); k < dim; k++)
					C[i*dim+j] += A[i*dim+k] * B[j+k*dim];
	
	
	}
	
	#else
	
		for (i=0; i < dim; i++)
			for (j=0; j < dim; j++)
				C[i*dim+j] = 0.0;
				
		
		for (i=0; i < (dim); i++)
			for (j=0; j < (dim); j++)
				for (k=(i+1); k < dim; k++)
					C[i*dim+j] += A[i*dim+k] * B[j+k*dim];
	#endif
}

void matmul_inf (float *A, float *B, float *C, int dim)
{
	int i, j, k;

	//inicializa matriz de resultado C
	for (i=0; i < dim; i++)
		for (j=0; j < dim; j++)
			C[i*dim+j] = 0.0;

	//computa la multiplicación optimizada
	for (i=1; i < dim; i++)
		for (j=0; j < dim; j++)
			for (k=0; k < i; k++)
				C[i*dim+j] += A[i*dim+k] * B[j+k*dim];
}

