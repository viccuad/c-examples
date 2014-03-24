#include "alarma.h"

//extern pthread_mutex_t alarma_mutex;
extern pthread_cond_t alarma_cond;
extern alarma_t *alarma_lista;
extern time_t alarma_actual;

/*
 * Esta rutina requiere que el invocador haya
 * bloqueado (LOCKED) el alarma_mutex.
 */
void alarma_inserta(alarma_t *alarma) {
	int estado;
	alarma_t **ultima, *sigue;
	/*
	 * Inserta la nueva alarma en la lista de alarmas
	 * ordenadas por tiempo de expiracion.
	 */
	ultima = &alarma_lista;
	sigue = *ultima;
	while (sigue != NULL) {
		if (sigue->tiempo >= alarma->tiempo) {
			alarma->enlace = sigue;
			*ultima = alarma;
			break;
		}
		ultima = &sigue->enlace;
		sigue = sigue->enlace;
	}
	/*
	 * Si alcanza el final de la lista, inserta la nueva
	 * alarma alli. ("sigue" es NULL, y "ultima" apunta
	 * al campo "enlace" de la ultima entrada, o a la
	 * cabecera de la lista).
	 */
	if (sigue == NULL) {
		*ultima = alarma;
		alarma->enlace = NULL;
	}
#ifdef DEBUG
	fprintf(stderr, "[lista: ");
	for (sigue = alarma_lista; sigue != NULL; sigue = sigue->enlace)
		fprintf(stderr, "%d(%d)[\"%s\"] ", (int)sigue->tiempo,
			(int)(sigue->tiempo - time (NULL)), sigue->mensaje);
	fprintf(stderr,"]\n");
#endif

	/* CODIGO
	 * Despierta a alarma_thread si no esta ocupado (es decir,
	 * si alarma_actual es 0, lo que indica que esta esperando
	 * trabajo), o si la nueva alarma va antes que la que esta
	 * siendo actualmente atendida por alarma_thread.
	 */
	if (alarma_actual == 0 || alarma->tiempo < alarma_actual) {
		if ((estado = pthread_cond_signal(&alarma_cond)) != 0)
			err_abort(estado, "Pthread_cond_signal: alarma_inserta\n" );
	}
}
