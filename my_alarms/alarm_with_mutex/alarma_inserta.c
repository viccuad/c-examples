#include "alarma.h"

extern alarma_t *alarma_lista;

void alarma_inserta(alarma_t *alarma) {
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
		*ultima = alarma;   // *ultima es el enlace que hay dentro de ultima (ultima->enlace)
		alarma->enlace = NULL;
	}
	#ifdef DEBUG
		fprintf(stderr, "[lista: ");
		for (sigue = alarma_lista; sigue != NULL; sigue = sigue->enlace)
			fprintf(stderr, "%d(%d)[\"%s\"] ", (int)sigue->tiempo,
				(int)(sigue->tiempo - time(NULL)), sigue->mensaje);
		fprintf (stderr, "]\n");
	#endif
}
