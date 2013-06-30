#include "alarma.h"

extern pthread_mutex_t alarma_mutex;
extern pthread_cond_t alarma_cond;
extern alarma_t *alarma_lista;
extern time_t alarma_actual;
extern int fin_solicitado;

/*
 * Rutina inicial del thread.
 */
void *alarma_thread (void *arg)
{
    alarma_t *alarma;
    struct timespec cond_tiempo;
    time_t ahora;
    int estado, expirado;


    /* 
     * Bloquea (lock) el mutex
     * al principio -- se desbloqueara (unlocked) durante las
     * esperas de condicion, de modo que el thread principal
     * podra insertar nuevas alarmas.
     */
	if ((estado = pthread_mutex_lock(&alarma_mutex)) != 0)
		err_abort(estado, "Lock hilo gestor\n");

    /*
     * Itera continuamente procesando solicitudes. El thread
     * desaparecera cuando el proceso termine. 
     */

    while (1) {

		if (fin_solicitado == 1 && alarma_lista == NULL) {
			pthread_exit(NULL);
		}
		
		/* 
		 * Si la lista de alarmas esta vacia, espera a que se inserte
	 	 * una nueva. Al poner alarma_actual a 0 se informa a la rutina
	 	 * alarma_inserta() que alarma_thread no esta ocupado.
         */
		while (alarma_lista == NULL && fin_solicitado != 1) { // no hay alarmas y no se haya solicitado salir
			alarma_actual = 0;
			if ((estado = pthread_cond_wait(&alarma_cond, &alarma_mutex)) != 0) // asi que permite que thread ppal se ejecute
				err_abort(estado, "Pthread_cond_wait hilo gestor\n");
		}

		/* 
		 * Toma la primera alarma; 
		 * Si hay que procesarla prepara una espera condicional con 
		 * temporizacion de la que saldra 
		 *   a) por tiempo expirado => informa de alarma cumplida
		 *   b) por señal de nueva alarma mas corta => reinsertar en la lista
		 */
		if (alarma_lista != NULL) {
			alarma = alarma_lista; // me cojo la primera
			alarma_lista = alarma_lista->enlace; // retiro la primera de la lista
			alarma_actual = alarma->tiempo;
			cond_tiempo.tv_sec = alarma_actual;
			cond_tiempo.tv_nsec = 0;
			ahora = time(NULL);
			if (alarma_actual - ahora <= 0) { // expirada (no hay que procesarla)
				cond_tiempo.tv_sec = 0;
				expirado = 1;
			}
			else { // hay que procesarla
				expirado = 0;
				estado = -1;
				while (estado != ETIMEDOUT && estado != 0) // vencida o alarma mas temprana (insertada por el principio)
					estado = pthread_cond_timedwait(&alarma_cond, &alarma_mutex, &cond_tiempo); // asi permite que thread ppal se ejecute
				switch (estado) {
					case 0: // se ha insertado una alarma mas corta
						alarma_inserta(alarma); // se reinserta
						break;
					case ETIMEDOUT: // se ha completado la espera
						expirado = 1;
						break;
					default:
						err_abort(estado, "Pthread_cond_timed_wait hilo gestor\n");
						break;
				}
			}
		
			if (expirado == 1) {
				printf("(%d) %s\n", alarma->segundos, alarma->mensaje);
				free (alarma);
			}
		}

		/* NOTA:
		 * Incluir como salida de depuracion los valores de la alarma
		 * que va a ser atendida
		 */
		#ifdef DEBUG
				fprintf(stderr, "%d(%d)[\"%s\"] ", (int)alarma->tiempo,
					(int)(alarma->tiempo - time(NULL)), alarma->mensaje);
				fprintf (stderr, "\n");
		#endif
 
    }
}

