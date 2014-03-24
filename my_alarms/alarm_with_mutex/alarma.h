#ifndef __alarma_h
#define __alarma_h

#include <pthread.h>
#include <time.h>
#include <err.h>
#include "errores.h"

/*
 *  La estructura "alarma" contiene el time_t (tiempo en segundos
 * transcurridos desde la EPOCA) para cada alarma, para que puedan ser
 * ordenadas. Almacenar el numero solicitado de segundos no seria
 * suficiente, ya que el "alarma_thread" no puede conocer cuanto tiempo
 * ha estado la solicitud en la lista.
 */
typedef struct alarma_tag {
	struct alarma_tag    *enlace;
	int                 segundos;
	time_t              tiempo;   /* segundos desde la EPOCA */
	char                mensaje[64];
} alarma_t;

void *alarma_thread (void *arg);
void alarma_inserta(alarma_t *alarma);

#endif
