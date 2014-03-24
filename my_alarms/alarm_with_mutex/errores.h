#ifndef __errors_h
#define __errors_h

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Define una macro que puede ser usada para salida de diagnostico.
 * Cuando se compila con -DDEBUG, da lugar a un printf aplicado a la
 * lista de argumnentos especificados. Cuando no se defina DEBUG
 * se expande a nada.
 */
#ifdef DEBUG
# define DPRINTF(arg) printf arg
# define DFPRINTF(arg) fprintf arg
#else
# define DPRINTF(arg)
# define DFPRINTF(arg)
#endif

/*
 * NOTA: la construccion "do {" ... "} while (0);" alrededor de las macros
 * err_abort y errno_abort permite que estas puedan ser usadas como si fueran
 * llamadas a funcion, inclusio en contextos donde un ';' final pudiera generar
 * una sentencia nula. Por ejemplo,
 *
 *      if (estado != 0)
 *          err_abort (estado, "mensaje");
 *      else
 *          return estado;
 *
 * no se compilaria si err_abort fuese una macro que acabase con "}", ya que
 * C no espera que despues de "}" venga ";". Como C si espera un ";"
 * despues del ")" en la construccion do...while, err_abort y  errno_abort
 * pueden ser usados como si fueran llamadas a funcion
 */
#define err_abort(codigo,texto) do { \
	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
		texto, __FILE__, __LINE__, strerror (codigo)); \
	abort (); \
	} while (0)
#define errno_abort(texto) do { \
	fprintf (stderr, "%s at \"%s\":%d: %s\n", \
		texto, __FILE__, __LINE__, strerror (errno)); \
	abort (); \
	} while (0)

#endif
