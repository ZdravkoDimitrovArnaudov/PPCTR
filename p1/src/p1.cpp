/**
 * @author Zdravko Dimitrov
 * @brief CLI que facilita abordar operaciones de suma,
 * resta y xor. El códido permite realizar ejecución paralela entre
 * los diferentes hilos que dispone el computador. Para ello, se indicarrá
 * por código el número de elementos (array Length) y la operación a efectuar.
 * El array se crea de forma dinámica inicializando cada elemento con el valor 
 * de su respectiva posición, en tipo flotante de doble precisión
 * @version 1.0
 * */

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <sys/time.h>
#include <ctype.h>

//#define FEATURE_LOGGER
#define DEBUG 
//#define FEATURE_OPTIMIZE

//creamos mutexes y cualquier variable condicional necesaria
std::mutex mutex_threads;
std::mutex mutex_threads_logger;
std::mutex mutex_logger_main;
std::condition_variable cv_thread_logger;
std::condition_variable cv_threads;


typedef enum operation {SUM=0, SUB ,XOR} t_operation;
typedef enum mode {single, multi} t_mode;

typedef struct args {
	int N;
	t_operation op;
	t_mode m;
	int n_threads;
	
}t_args;

typedef struct data{
	double *array_datos;
	int size;
	double resultado; //resultado globlal
	
	/*En caso de usar optimize*/
	std::atomic<double> resultado_atm;
	
	//Para delimitar área de trabajo de thread
	long elementos_thread;
	
	/*en caso haber exceso*/
	bool exceso_gestionado; 
	int tam_exceso;
	
	/*en caso de emplearse logger*/
	std::thread logger;
	bool thread_ready;
	double *array_logger;
	double resultado_parcial;
	double resultado_logger;
	int identificador_thread;
	
	/*comunicación entre main y logger*/
	std::condition_variable cv_logger_main;
	bool logger_ready;
	bool main_ready;
	
}t_data;


/**
 * Inicializa los campos de la estructura t_data.
 * @param t_data *datos: puntero a la estructura datos.
 * @param t_args *argumentos: puntero a la estructura argumentos.
 * 
 * */
void inicializa_datos(t_data *datos, t_args *argumentos);
 
 /**
  * Inicializa los campos de la estructura t_data si se emplea FEATURE_LOGGER.
  * @param t_data *datos: puntero a la estructura datos.
  * @param t_args *argumentos: puntero a la estructura argumentos.
  * */
void inicializa_logger(t_data *datos, t_args *argumentos);

/**
 * Muestra por pantalla informaicón de ayuda si el usuario la precisa
 * con "--help"
 * 
 * */
void muestra_ayuda();

/**
 * Función en la que trabaja el hilo logger para almacenar recoger los resultados
 * parciales de los threads, acumularlos y comunicarlo al main.
 * @param t_data *datos: puntero a la estructura de datos 
 * @param t_args *argumentos: puntero a la estructura de argumentos 
 * */
void procesa_threads (t_data *datos, t_args *argumentos);

/**
 * Función en la que trabajan los threads si se seleccióna la
 * operación de suma.
 * @param int start: indice de comienzo de trabajo en el array
 * @param int id: identificador el thread establecido en el bucle de creación.
 * @param t_data *datos: puntero la estructura de datos
 * */
void procesa_SUM(int start, int id, t_data *datos);

/**
 * Función en la que trabajan los threads si se seleccióna la
 * operación de resta.
 * @param int start: indice de comienzo de trabajo en el array
 * @param int id: identificador el thread establecido en el bucle de creación.
 * @param t_data *datos: puntero la estructura de datos
 * */
void procesa_SUB(int start, int id, t_data *datos);

/**
 * Función en la que trabajan los threads si se seleccióna la
 * operación de xor.
 * @param int start: indice de comienzo de trabajo en el array
 * @param int id: identificador el thread establecido en el bucle de creación.
 * @param t_data *datos: puntero la estructura de datos
 * */
void procesa_XOR(int start, int id, t_data *datos);

int main(int argc, char *argv[]){
	
	//estructura de argumentos del programa
	t_args argumentos={};
	
	//estructura de datos del programa
	t_data datos={};
	
	struct timeval start,end; //gettimeday()
	
	//para multithread
	bool resultado_correcto = false; 
	
	
	//GESTION ERRORES..
	
	//numero de argumentos(max y min)
	if (argc < 2 || argc > 5){
		printf("Argumentos insuficientes o no válidos.\n");
		return -1;
	}
	
	//Ayuda del programa
	if (argc == 2 && (strcmp(argv[1], "--help") == 0)){
		muestra_ayuda();
		return -1;
		
	}
	 
	 //N size arary
	 long TA = atol(argv[1]);
	 if(TA <= 0){
		 printf ("El tamaño del array no es válido\n");
		 return -1;
		 
	 } else {
		 //anotamos el tamaño del array
		 argumentos.N = TA;
		 
	 }
	 
	
	 //operation
	 if ((strcmp(argv[2], "SUM"))==0){
		 argumentos.op = SUM;
		 
	 } else if ((strcmp(argv[2], "SUB"))==0){
		 argumentos.op = SUB;
		 
	 } else if ((strcmp(argv[2], "XOR"))==0){
		 argumentos.op = XOR;
		 
	 } else {
		 printf ("El tipo de operación no es válido (únicamente SUM,SUB o XOR).\n");
		 return -1;
		 
	 }
	 
	
	 //mode
	 if ((strcmp(argv[3], "single"))==0){
		 argumentos.m=single;
		 
	 } else if ((strcmp(argv[3], "multi"))==0){
		 argumentos.m = multi;
		 
	 } else {
		 printf ("El modo de operación no es válido (únicamente single o multi).\n");
		 return -1;
		 
	 }
	 
	
	 //n_threads
	 if (argumentos.m == single && argc == 5){
		 printf("En este modo de operación solo puede funcionar el thread principal.\n");
		 return -1;
		 
	 } else if (argumentos.m == multi && argc == 4){
		 printf ("No se han indicado el número de threads para este modo de operación.\n");
		 return -1;
		 
	 }
	 
	 if (argumentos.m == multi){ //si el modo es multi, recogemos el numero de threads
		 
		int NT = atoi(argv[4]);
		if (NT < 0){
			printf ("El número de threads introducidos no es válido.\n");
			return -1;
		 
		} else {
			//anotamos el número de threads
			argumentos.n_threads = NT;
		 
		}
	 
	}
	 
	
	 /*Una vez disponemos de nuestros argumentos, inicializamos los campos necesarios 
	  * de la estructura de datos.*/
	 inicializa_datos (&datos, &argumentos);
	 
	 //si empleamos logger
	 #ifdef FEATURE_LOGGER
		inicializa_logger(&datos, &argumentos);
	 #endif
	
	
	
	  /*PROCESADO ARRAY*/
	 if (argumentos.m == single) { //single
		 
		 if (argumentos.op==SUM) { //SUM
			 
			 gettimeofday(&start,NULL); //medimos tiempo inicial
			 
			 //calculamos operación
			 for (int i = 0; i < argumentos.N; ++i){
				 datos.resultado += datos.array_datos[i];
				  
			  }
			  
			gettimeofday(&end, NULL); //medimos tiempo final
			
			//Mostramos resultados
			std::cout << "El resultado de la operación suma es: " << datos.resultado <<std::endl;
			printf("El tiempo empleado en la operación son: %ld microsegundos\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
		
		 } else if (argumentos.op==SUB) {//SUB
			 
			 gettimeofday(&start,NULL); 
			 for (int i = 0; i < argumentos.N; ++i){
				 datos.resultado -= datos.array_datos[i];
				  
			 }
			 gettimeofday(&end, NULL); 
			
			 std::cout << "El resultado de la operación resta es: " << datos.resultado <<std::endl;
		     printf("El tiempo empleado en la operación son: %ld microsegundos\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
			 
			 
		 } else { //XOR
			 
			 gettimeofday(&start,NULL);
			 for (int i = 0; i < argumentos.N; ++i){
				  datos.resultado = (long) datos.resultado^ (long)datos.array_datos[i];
				  
			  }
			  
			 gettimeofday(&end, NULL);
			 std::cout << "El resultado de la operación xor es: " <<datos.resultado <<std::endl;
			 printf("El tiempo empleado en la operación son: %ld microsegundos\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
			 
		 }
		 
	 } else { //multi
		 
		 /*Tareas previas antes de computación paralela*/
		 std::thread threads[argumentos.n_threads]; 
		 datos.elementos_thread = (long) argumentos.N / argumentos.n_threads; //cantidad de trabajo para cada thread
		 
		 datos.tam_exceso = datos.size - (datos.elementos_thread*argumentos.n_threads);
		 if (datos.tam_exceso == 0){
			 //la operación es exacta y no debe calcularse ningún exceso
			 datos.exceso_gestionado = true; 
			 
		 }
			 
		 
			  			   
		#ifdef FEATURE_LOGGER
			datos.logger = std::thread(procesa_threads, &datos, &argumentos); //creamos thread logger
		#endif
		  
		gettimeofday(&start,NULL); //medimos tiempo inicial
			  
		/*Creamos los threads y les asignamos el area de trabajo en el que deben computar en función del tipo de operación*/
		for (int i=0; i<argumentos.n_threads; ++i){
				  
			if (argumentos.op == SUM){ 
				threads[i] = std::thread(procesa_SUM, i * datos.elementos_thread, i, &datos);
				  
			} else if (argumentos.op == SUB){
				threads[i] = std::thread(procesa_SUB, i * datos.elementos_thread, i, &datos);
				  
			} else{
				threads[i] = std::thread(procesa_XOR, i * datos.elementos_thread, i, &datos);
				    
			}
			  
		}
			  
		//sincronizamos todos los threads
		for (int i = 0; i < argumentos.n_threads; ++i){
			threads[i].join();
				 
		}
			  
		gettimeofday(&end,NULL); //medimos tiempo final 
			  
		#ifdef FEATURE_LOGGER //si hemos usado logger
				  
		std::unique_lock<std::mutex> ulk(mutex_logger_main);	
	
		if(datos.main_ready == false){
			datos.cv_logger_main.wait(ulk); //dormir hasta que el logger lo despierte
		}
				  
		ulk.unlock(); //liberamos mutex
		datos.logger.join(); //sincronizamos con logger
				
		//Mostramos resultados
		#ifdef FEATURE_OPTIMIZE
		if (datos.resultado_atm == datos.resultado_logger){
			resultado_correcto = true;
		}
				 
		#else
		if (datos.resultado == datos.resultado_logger){
			resultado_correcto = true;
		}
				 
		#endif
				 
		if (resultado_correcto){
			printf("El resultado de los hilos obtenido por logger es  correcto.\n");
		} else {
			printf("El resultado de los hilos obtenido por logger es incorrecto.\n");
		}
		
		std::cout<< "Valor recibido por logger: "<< datos.resultado_logger<<std::endl; 
				
				  
		#endif
			
		#ifdef FEATURE_OPTIMIZE
			std::cout<<"Valor recibido por los threads: "<< datos.resultado_atm<<std::endl;
		#else
			std::cout<<"Valor recibido por los threads: "<< datos.resultado<<std::endl;
		#endif
			
		//Mostrar tiempo de ejcución
		printf("El tiempo empleado en la operación son: %ld microsegundos\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
		
			  
			
	
}
		 
	free(datos.array_datos); //liberamos malloc
	
	#ifdef FEATURE_LOGGER
		free(datos.array_logger);
	#endif 
	
	return 0;
}



void inicializa_datos(t_data *datos, t_args *argumentos){
	
	//reservamos espacio para el array
	 datos->array_datos = (double*) malloc(argumentos->N*sizeof(double)); //malloc para array de elementos
	 
	 //inicializamos el array de elementos
	 for (int i = 0; i < argumentos->N; ++i){
		datos->array_datos[i] = i; 
	 }
	 
	 //inicalizamos variable atómica en caso de tener que usarla.
	 #ifdef FEATURE_OPTIMIZE
	 datos->resultado_atm  = 0.00;
	 #endif
	 
	 //inicializamos otros campos necesarios de la estructura
	 datos->size = argumentos->N;
	 datos->exceso_gestionado = false;
	 datos->tam_exceso = 0;
	 datos->resultado = 0.00;
	 datos->elementos_thread = 0.00;
	
}

void inicializa_logger(t_data *datos, t_args *argumentos){
	
	//inicializamos aquellos campos cuando se emplee logger
		datos->array_logger = (double*) malloc(argumentos->n_threads*sizeof(double)); //malloc para array de resultados parciales para el logger
		datos->thread_ready = false;
		datos->logger_ready = true;
		datos->main_ready = false;
		datos->resultado_logger = 0.00;
		datos->resultado_parcial = 0.00;
		datos->identificador_thread = - 1;
		
	
}

void muestra_ayuda(){
	printf ("#AYUDA");
	printf ("-Argumento 1: tamaño N de array.\n");
	printf ("-Argumento 2: tipo de operación (SUM, SUB o XOR).\n");
	printf ("-Argumento 3: Modo de ejecución.\n");
	printf ("	+Si desea modo single-thread, use: single\n");
	printf ("	+Si desea modo multi-thread, use: multi\n");
	printf ("-Argumento 4: Número de threads.\n");
	printf ("	+si ha especificado multi, introduzca número de threads\n");
	printf ("-Ejemplos de ejecución: '5000 SUM single' , '40000 XOR multi 8'\n");
	printf ("\n");
	
}


void procesa_SUM(int start, int id, t_data *datos){
	
	//resultado parcial del thread
	double thread_suma = 0.00;
	
	//operamos en la sección delimitada para el array
	for (int i = 0; i < datos->elementos_thread; ++i){
		thread_suma += datos->array_datos[start + i];
		
	}
	
	#ifdef DEBUG
		printf ("El thread %d termina la operación.\n", id);
	#endif
	
	//al finalizar de computar vemos si podemos calcular el exceso, si lo hay
	{
		std::lock_guard<std::mutex> lock(mutex_threads); //mutex lock
		if (datos->exceso_gestionado == false ){  
			
			//se notifica 
			datos->exceso_gestionado = true;
		
			//debemos recorrer la sección restante del array y operarla.
			for (int i = 0; i < datos->tam_exceso; ++i){
				thread_suma += datos->array_datos[(datos->size - datos->tam_exceso)+i];
				
			}
			
			#ifdef DEBUG
			printf ("El thread %d acaba primero y termina el exceso de trabajo.\n", id);
		
			#endif
			
		}	
		
	}//mutex unlock
	
	//actualizamos el resultado global con el parcial de cada thread
	#ifdef FEATURE_OPTIMIZE
	datos->resultado_atm = datos->resultado_atm + thread_suma;
	#else
	{
		std::lock_guard<std::mutex> lock(mutex_threads);
		datos->resultado += thread_suma; //actualizamos resultado global con el parcial
	
	}//libera mutex	
	#endif	
	
	
	#ifdef DEBUG
	printf ("El thread %d actualiza el resultado y finaliza.\n", id);
		
	#endif
	
	
	
	//comunicación con logger
	#ifdef FEATURE_LOGGER
		
		/*Garantizamos que mientras el logger esté operando y no haya terminado
		 * de gestionar el resultado parcial del anterior thread, el siguiente 
		 * debe estar a la espera.*/
		std::unique_lock<std::mutex> ulk(mutex_threads_logger);
		if (datos->logger_ready == false){ 
			#ifdef DEBUG
			printf ("El thread %d espera a transferir su resultado al logger.\n", id);
			#endif
			cv_threads.wait(ulk);
			
		}//tomamos mutex
		
		datos->logger_ready = false; //el logger volverá a estar disponible cuando termine
		
		//modificaciones oportunas
		datos->resultado_parcial = thread_suma;
		datos->identificador_thread = id;
		
		#ifdef DEBUG
		printf ("El thread %d ha comunicado al logger su resultado parcial e id.\n", id);
		
		#endif
		
		//indicamos que el thread está listo
		datos->thread_ready = true;
		
		#ifdef DEBUG
		printf ("El thread %d despertará al logger y liberará el mutex.\n", id);
		
		#endif
		
		cv_thread_logger.notify_all(); //desperamos al logger (porque su condición se cumplirá y la del thread siguiente no).
		
		//liberamos el mutex
		ulk.unlock();
		
			
	#endif
	
	
 }



void procesa_SUB(int start, int id, t_data *datos){
	
	double thread_resta = 0.00;
	for (int i = 0; i < datos->elementos_thread; ++i){
		thread_resta -= datos->array_datos[start + i];
		
	}
	
	#ifdef DEBUG
		printf ("El thread %d termina la operación.\n", id);
		
	#endif 
	
	{ //mutex lock
		std::lock_guard<std::mutex> lock(mutex_threads); //mutex lock
		if (datos->exceso_gestionado == false ){  
			datos->exceso_gestionado = true;
			#ifdef DEBUG
			printf ("El thread %d acaba primero y termina el exceso de trabajo.\n", id);
		
			#endif
			for (int i = 0; i < datos->tam_exceso; ++i){
				thread_resta -= datos->array_datos[(datos->size - datos->tam_exceso)+i];
				
			}
		}	
		
	}
	//actualizamos el resultado global con el parcial de cada thread
	#ifdef FEATURE_OPTIMIZE
	datos->resultado_atm = datos->resultado_atm + thread_resta;
	#else
	{
		std::lock_guard<std::mutex> lock(mutex_threads);
		datos->resultado -= thread_resta; //actualizamos resultado global con el parcial
	
	}//libera mutex	
	#endif	
	
	
	#ifdef DEBUG
	printf ("El thread %d actualiza el resultado y finaliza.\n", id);
		
	#endif
	

	#ifdef FEATURE_LOGGER
		
		std::unique_lock<std::mutex> ulk(mutex_threads_logger);
		if(datos->logger_ready == false){ 
			#ifdef DEBUG
			printf ("El thread %d espera a transferir su resultado al logger.\n", id);
		
			#endif 
			cv_threads.wait(ulk);
		}
		
		datos->logger_ready = false; 
		datos->resultado_parcial = thread_resta;
		datos->identificador_thread = id;
		
		#ifdef DEBUG
		printf ("El thread %d ha comunicado al logger su resultado parcial e id.\n", id);
		
		#endif
		datos->thread_ready = true;
		
		#ifdef DEBUG
		printf ("El thread %d despertará al logger y liberará el mutex.\n", id);
		
		#endif
		
		cv_thread_logger.notify_all(); 
		ulk.unlock();
		
			
	#endif
	
}

void procesa_XOR(int start, int id, t_data *datos){

	double thread_xor = 0.00;
	for (int i = 0; i < datos->elementos_thread; ++i){
		thread_xor = (long)thread_xor ^ (long)datos->array_datos[start + i];
		
	}
	
	#ifdef DEBUG
		printf ("El thread %d termina la operación.\n", id);
	
	#endif
	
	{//mutex lock
		std::lock_guard<std::mutex> lock(mutex_threads);
		if (datos->exceso_gestionado == false ){  
			datos->exceso_gestionado = true;
			#ifdef DEBUG
			printf ("El thread %d acaba primero y termina el exceso de trabajo.\n", id);
		
			#endif 	
			for (int i = 0; i < datos->tam_exceso; ++i){
				thread_xor = (long)thread_xor ^ (long)datos->array_datos[(datos->size - datos->tam_exceso)+i];
				
			}
		}	
		
	}

	//actualizamos el resultado global con el parcial de cada thread
	#ifdef FEATURE_OPTIMIZE
	datos->resultado_atm = (long)datos->resultado_atm ^ (long)thread_xor; 
	#else
	{
		std::lock_guard<std::mutex> lock(mutex_threads);
		datos->resultado = (long)datos->resultado ^ (long)thread_xor; 	
	
	}//libera mutex	
	#endif	
	
	
	#ifdef DEBUG
	printf ("El thread %d actualiza el resultado y finaliza.\n", id);
		
	#endif
	
	#ifdef FEATURE_LOGGER
		
		std::unique_lock<std::mutex> ulk(mutex_threads_logger);
		if (datos->logger_ready == false){ 
			#ifdef DEBUG
			printf ("El thread %d está a la espera de transferir su resultado al logger.\n", id);
		
			#endif
			cv_threads.wait(ulk);
			
		}
			
		datos->logger_ready = false; 
		datos->resultado_parcial = thread_xor;
		datos->identificador_thread = id;
		
		#ifdef DEBUG
		printf ("El thread %d ha comunicado al logger su resultado parcial e id.\n", id);
		
		#endif
		datos->thread_ready = true;
		
		#ifdef DEBUG
		printf ("El thread %d despertará al logger y liberará el mutex.\n", id);
		
		#endif
		
		cv_thread_logger.notify_all(); 
		ulk.unlock();
			
	#endif
	
}


void procesa_threads(t_data *datos, t_args *argumentos){
	
	 int iterador = 0;
	 
	 //Mientras no acaben todos los threads de pasar sus resultados parciales
	 while(iterador < argumentos->n_threads){ 
		 
		 std::unique_lock<std::mutex> ulk(mutex_threads_logger);
		 if(datos->thread_ready == false){
			 cv_thread_logger.wait(ulk);
			 
		 }//mutex lock
		 
		 #ifdef DEBUG
		printf ("El logger ha sido despertado y comenzará almacenando el resultado  parcial del thread %d.\n", datos->identificador_thread);
		
		#endif
		 
		 //guardamos el resultado parcial en la posición del array correspondiente
		 datos->array_logger[datos->identificador_thread] = datos->resultado_parcial;
		 
		#ifdef DEBUG
		printf ("El logger almacena el resultado: %f en la posicón %d del array.\n", datos->resultado_parcial, datos->identificador_thread);
		#endif 
		 
		 //actualizamos variables
		 iterador ++;
		 datos->thread_ready = false; //para que pueda volver a llamarse
		 datos->logger_ready = true;
		 
		 #ifdef DEBUG
		printf ("El logger está listo para ser despertado de nuevo.\n");
		
		#endif
		 
		 cv_threads.notify_one(); //despertamos a un thread que se encuentre a la espera
		 ulk.unlock();
		 
	 }
	 
	 /*Según la operación escogida, acumularemos el resultado de distinta forma.
	  * Mostramos el resultado parcial de cada thread en orden de creación*/
		for (int i = 0; i < argumentos->n_threads; ++i){
			 
			 if (argumentos->op == SUM){
				datos->resultado_logger += datos->array_logger[i]; //si la operación es suma
				
			} else if (argumentos->op == SUB){
				datos->resultado_logger -= datos->array_logger[i]; //si la operación es resta
				
			} else {
				datos->resultado_logger = (long)datos->resultado_logger ^ (long)datos->array_logger[i]; //si la operación es xor
				
			}
			printf ("El thread %d ha obtenido un resultado parcial de: %f\n", i, datos->array_logger[i]);
			
		 } 
		 
		 //el resultado del logger está actualizado. El main tiene que leer el valor de dicho resultado.
		{
			std::lock_guard<std::mutex> lock(mutex_logger_main);
			datos->main_ready = true;
		}
		datos->cv_logger_main.notify_all(); //despertamos al main
	 
		 
}
