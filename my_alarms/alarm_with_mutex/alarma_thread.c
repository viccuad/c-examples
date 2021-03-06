/*
 * Rutina inicial del thread.
 */

#include "alarma.h"

extern pthread_mutex_t alarma_mutex;
extern alarma_t *alarma_lista;

extern int fin_solicitado;

void *alarma_thread (void *arg)
{
	alarma_t *alarma;
	int tiempo_espera;
	time_t ahora;
	int estado;

	/*
	* Itera continuamente procesando solicitudes. El thread
	* desaparecera cuando el proceso termine.
	*/
	while (1) {

		if (fin_solicitado == 1 && alarma_lista == NULL) {
			pthread_exit(NULL);
		}

		// dentro de una seccion critica
		if ((estado = pthread_mutex_lock(&alarma_mutex)) != 0)
			err_abort(estado, "Lock hilo gestor\n");

		/*
		* Si la lista de alarmas esta vacia, espera durante un segundo.
		* Esto permite que el thread principal se ejecute y pueda admitir
		* otra solicitud. Si la lista no esta vacia, retira la primera entrada
		* Calcula el numero de segundos que debe esperar -- si el resultado
		* es menor que 0 (el tiempo ya ha pasado), pone el tiempo_espera a 0.
		*/
		alarma = alarma_lista; // me cojo la primera
		if (alarma_lista == NULL) // no hay alarmas
			tiempo_espera = 1; // asi que permite que thread ppal se ejecute
		else { // hay alarmas
			alarma_lista = alarma_lista->enlace; // retiro la primera de la lista
			ahora = time(NULL);
			if ((tiempo_espera = alarma->tiempo - ahora) <= 0) // expirada
				tiempo_espera = 0;
		}

		/* NOTA
		 * Incluir como salida de depuracion los valores de la alarma
		 * que va a ser atendida
		 */
		#ifdef DEBUG
			fprintf(stderr, "%d(%d)[\"%s\"] ", (int)alarma->tiempo,
				(int)(alarma->tiempo - time(NULL)), alarma->mensaje);
			fprintf (stderr, "\n");
		#endif

		/*
		 * Desbloquea (unlock) el mutex antes de pasar a esperar de modo que el
		 * thread principal pueda bloquearlo (lock) para insertar una nueva
		 * solicitud. Si el tiempo_espera es 0, invoca a sched_yield, dando
		 * al thread principal una oportunidad de ejecutarse si ha recibido
		 * entrada del usuario, y sin demorar el mensaje en caso de que no haya
		 * recibido entrada.
		 */
		if ((estado = pthread_mutex_unlock(&alarma_mutex)) != 0)  // fin seccion critica
			err_abort(estado, "Unlock hilo gestor\n" );
		if (tiempo_espera == 0) // alarma expirada: ceder CPU
			sched_yield();
		else
			sleep(tiempo_espera);
		/*
		 * Si ha expirado una alarma, imprime el mensaje y libera la estructura
		 */
		if (alarma != NULL) {
			printf("(%d) %s\n", alarma->segundos, alarma->mensaje);
			free (alarma);
		}
	}
}

