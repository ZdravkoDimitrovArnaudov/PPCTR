#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>

#define PARALLEL
//#define DEBUG
//#define IMPRIME //para imprimir imágenes por pantalla

void fgauss (int *, int *, int, int);

int main(int argc, char *argv[]) {

   FILE *in;
   FILE *out;
   int i, j, size, seq = 8;
   int **pixels, **filtered;


   //para medir el tiempo
   double start, end;


   if (argc == 2) seq = atoi (argv[1]);

//   chdir("/tmp");
   in = fopen("movie.in", "rb");
   if (in == NULL) {
      perror("movie.in");
      exit(EXIT_FAILURE);
   }

   out = fopen("movie.out", "wb");
   if (out == NULL) {
      perror("movie.out");
      exit(EXIT_FAILURE);
   }

   int width, height;

   fread(&width, sizeof(width), 1, in);
   fread(&height, sizeof(height), 1, in);

   fwrite(&width, sizeof(width), 1, out);
   fwrite(&height, sizeof(height), 1, out);

   pixels = (int **) malloc (seq * sizeof (int *));
   filtered = (int **) malloc (seq * sizeof (int *));

   for (i=0; i<seq; i++)
   {
      pixels[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
      filtered[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
   }

   //variables necesarias para paralelizar
   i = 0;
   int contador_iter = 0;
   bool termina = false;

   //medir tiempo inicial
   start = omp_get_wtime();

   #ifdef PARALLEL  //paralelo

   #ifdef DEBUG
   printf ("Ejecución paralela:\n");
   #endif

   #pragma omp parallel shared (pixels, height, width, in, out, filtered, seq, i, contador_iter) num_threads(6)
   {

	  #pragma omp single
		{
			bool termina = false;

		do {

			//mientras queden imagenes por leer, acumular hasta seq
			while (i < seq && termina == false){
			size = fread(pixels[i], (height+2) * (width+2) * sizeof(int), 1, in);

			if (size){
				i++;

			} else {
				termina = true;
			}

			}
			
			
		  
			//para cada imágen leida, filtrarla en modo tarea
			for (int j = 0; j < i; ++j){
				#pragma omp task 
				{
				fgauss (pixels[j], filtered[j], height, width);

				#ifdef IMPRIME
				int n_t = omp_get_thread_num();
				printf ("Soy el thread: %d ejecutando la imagen: %d\n", n_t, contador_iter + j);
				#endif
			}


			}

			#pragma omp taskwait //espero a que todos acaben

			//escribímos las imágenes en el fichero de salida
			for (int j = 0; j < i ; ++j){
				fwrite(filtered[j], (height+2) * (width + 2) * sizeof(int), 1, out);

				#ifdef DEBUG
				printf ("Imagen terminada.\n");
				#endif

			}

			//inicializamos i y bool
			contador_iter = contador_iter + i;
			i = 0;
			termina = false;

		} while (!feof(in));


	} //fin single

} //fin parallel

	#else //secuencial

   #ifdef DEBUG
   printf ("Ejecución secuencial:\n");
   #endif


    do
   {

	size = fread(pixels[i], (height+2) * (width+2) * sizeof(int), 1, in);

	if (size)
	{

		fgauss (pixels[i], filtered[i], height, width);
		fwrite(filtered[i], (height+2) * (width + 2) * sizeof(int), 1, out);
		#ifdef DEBUG
		printf ("Imagen terminada.\n");
		#endif
	}



	} while (!feof(in));




	#endif

   //medir tiempo final
   end = omp_get_wtime();


   printf ("Tiempo de ejecución en s: %f\n", end-start);

   for (i=0; i<seq; i++)
   {
      free (pixels[i]);
      free (filtered[i]);
   }

   free(pixels);
   free(filtered);


   fclose(out);
   fclose(in);

   return EXIT_SUCCESS;
}



void fgauss (int *pixels, int *filtered, int heigh, int width) {
	int y, x, dx, dy;
	int filter[5][5] = {{1, 4, 6, 4, 1}, {4, 16, 26, 16, 4},{ 6, 26, 41, 26, 6}, {4, 16, 26, 16, 4}, {1, 4, 6, 4, 1}};
	int sum;

	for (x = 0; x < width; x++) {
			for (y = 0; y < heigh; y++){

				sum = 0;
				for (dx = 0; dx < 5; dx++)
					for (dy = 0; dy < 5; dy++)
						if (((x+dx-2) >= 0) && ((x+dx-2) < width) && ((y+dy-2) >=0) && ((y+dy-2) < heigh))
							sum += pixels[(x+dx-2)&&(y+dy-2)] * filter[dx][dy];
				filtered[x*heigh+y] = (int) sum/273;
	
			}
		}

   
}
