#include <stdio.h> 		//para standard I/O
#include <unistd.h>  	//para POSIX API, getopt() getopt tutorial: http://www.ibm.com/developerworks/aix/library/au-unix-getopt.html
#include <stdlib.h> 	//para EXIT_SUCCESS and EXIT_FAILURE  (portabilidad)
#include <utmp.h>		//para el path de utmp y los structs utmp
#include <stdbool.h> 	//ansi C99 estandar (si no, se puede hacer typedef enum { false, true } bool;)
#include <errno.h>		//para errno
#include <sys/stat.h>	//para struct stat
#include <time.h>		//para localtime, strftime

extern int errno;  /* Guarda el último valor de salida de llamadas al sistema o
					funciones de librerias. perror() usará ese int para devolver una
					cadena de caracteres informando de que tipo de error es */


/* my_who1
 * soporta los siguientes argumentos:
 *
 * -H incluye una cabecera de introducción a la lista de datos.
 * -h muestra un recordatorio de uso de la orden.
 *
 * optString le dice a getopt() que opciones de argumentos soportar
 */


// Para guardar las opciones de los argumentos
	struct globalArgs_t {
		char *nombre_programa;
		bool cabecera;				/* opcion -H */
		char *inputFile;			/* input file */
		int numInputFiles;			/* num de ficheros de entrada */
	} globalArgs;

	// Informa a getopt de que opciones se procesan y cuales necesitan un argumento
	static const char *optString = "Hh?";

// Variables globales
static char const *formato_tiempo = "%b %e %H:%M";   // para la llamada a strftime()
				     //   %b + " " + %e + " " + %H + : +%M + '\0'
static int const anchura_formato_tiempo =  3 +  1  +  2 +  1  +  2 + 1 + 2 +  1;


//****************************************************************************//


// Mostrar uso de programa y salir
static void
mostrar_uso( void )
{
	fprintf(stderr, "Uso: %s [-H | -h] [ fichero ] ...\n", globalArgs.nombre_programa);
	fprintf(stderr, "donde\n");
	fprintf(stderr, "-H \t incluye una cabecera de introducción a la lista de datos\n");
	fprintf(stderr, "-h \t muestra un recordatorio de uso de la orden\n");
	fprintf(stderr, "\nSi no se especifica FICHERO, usa %s. Es habitual el uso ", UTMP_FILE);
	fprintf(stderr, "de %s como FICHERO.\n", WTMP_FILE);
	fflush(stderr); //vacia el buffer a stderr, por si acaso
	exit(EXIT_FAILURE);
}

static void
print_cabecera (void)
{
	printf("%-13s", "" );

	printf("%-12s", "NOMBRE" ); printf("\t");

	printf("%-8.8s", "LINEA" ); printf("\t");

	printf("%-25.24s", "TIEMPO" ); printf("\t");

	printf("%-4.4s", "ID"); printf("\t");

	printf("%-15.15s", "HOST"); printf("\t");

	printf("%-6.6s", "SALIDA");

	printf("\n");
}


/*  Devuelve un string con la fecha/hora.
	hace malloc de timestr, que es devuelto por return
	(acordarse de hacer un free cuando no se utilice)
    return tiempo formateado
*/
static char *
string_tiempo (const struct utmp *utmp_ent)
{
	/* NOTAS
		struct tm* localtime(const time_t* t) __THROW;	 =>   devuelve un struct tm a partir de un time_t (el cual suele ser un long int)
		strftime(...);		=>  devuelve en el 1er parámetro un string con el tiempo con formato blalbalblalbalblabla

	   USO
		Sacar el struct tm* con la funcion localtime
		Sacar el string con el tiempo con strftime
	 */

	// Obtener el tiempo en segundos
	long int time = utmp_ent->ut_time;
	// Reservar espacio para la cadena que se devolvera
	char *timestr = malloc( sizeof(char)*(anchura_formato_tiempo+1) ); //+1 para el caracter '\0' de fin de string

	// Pasarlo a struct tm*
	struct tm *time3 = localtime(&time);

	// Llamar a la funcion strftime pasandole como 4º parámetro un struct tm*
	strftime(timestr, anchura_formato_tiempo, formato_tiempo, time3);

	return timestr;
}


/* Explora el array de registros *utmp_buf, que debería tener n entradas
*/
static void
explorar_entradas (size_t n, const struct utmp *utmp_buf)
{
	if (globalArgs.cabecera) print_cabecera();

	// Recorremos las entradas
	int i;
	for (i = 1; i<=n; i++){

		// Imprimimos ut_type
		short int tipo = utmp_buf[i].ut_type;    //info(que no importa): valgrind --tool=memcheck ./my_who1. si es de tipo empty, have invalid read
		switch (tipo){
			case 0:
			  printf("%-13s", "(empty)");
			  break;
			case 1:
			  printf("%-13s", "(runlevel)");
			  break;
			case 2:
			  printf("%-13s", "(boottime)");
			  break;
			case 3:
			  printf("%-13s", "(newtime)");
			  break;
			case 4:
			  printf("%-13s", "(oldtime)");
			  break;
			case 5:
			  printf("%-13s", "(initprocs)");
			  break;
			case 6:
			  printf("%-13s", "(loginprocs)");
			  break;
			case 7:
			  printf("%-13s", "(userprocs)");
			  break;
			case 8:
			  printf("%-13s", "(deadprocs)");
			  break;
			case 9:
			  printf("%-13s", "(accounting)");
			  break;
			default:
			  printf("%-13s", "(no tipo)");
			  break;
		}//del while


		if (tipo == 0) printf("\n"); // nos saltamos una vuelta del for para que no escriba nada mas si es empty
		else { // imprimimos informacion solo si no es de tipo 0, es decir, si tiene tipo

			// Imprimimos ut_user
			const char *usuario = utmp_buf[i].ut_user;  //ponemos const ya que utmp_buf entra como const a la funcion
			printf("%-15.15s", usuario);
			printf("\t");

			// Imprimimos ut_line
			const char *linea = utmp_buf[i].ut_line;
			printf("%-8.8s", linea);
			printf("\t");

			// Imprimimos ut_time
			char *tiempo_formateado = string_tiempo(&utmp_buf[i]);	//guardamos en puntero para poder hacer luego un free
			printf("%-25.24s", tiempo_formateado);
			printf("\t");
			free(tiempo_formateado); //liberamos la memoria reservada para *tiempo_formateado por string_tiempo()

			// Imprimimos ut_id
			/* el id que se imprime son dos caracteres raros! para todos los procesos que no son tipo login
			 esto se puede arreglar, no imprimiendo el id de los procesos que no son de tipo login
			*/
			const char *id = utmp_buf[i].ut_id;
			printf("%-4.4s", id);
			printf("\t");

			// Imprimimos ut_host
			const char *host = utmp_buf[i].ut_host;
			printf("%-15.15s", host);
			printf("\t");

			// Imprimimos ut_exit
			short int termination  = utmp_buf[i].ut_exit.e_termination;
			short int exit_ut = utmp_buf[i].ut_exit.e_exit;	// exit es palabra reservada
			printf("term=%-2d", termination);
			printf("salida=%-2d", exit_ut);
			printf("\n");

		}//del if

	}//del for
}


/* Lee el fichero especificado y almcena su contenido
   en un array de STRUCT UTMP
   hace malloc de *utmp_buf, que se devuelve por parametros
   (acordarse de hacer free cuando no se utilice)
   return estado de finalizacion
*/
static int
leer_utmp(char *fichero, int *n_entradas, struct utmp **utmp_buf)
{
	FILE* fd;
	if ((fd = fopen(globalArgs.inputFile, "r")) == NULL) 		//abrimos el inputFile para lectura
	perror("Error leer_utmp()"), exit(EXIT_FAILURE);

	struct stat fichero_stat;
	stat(fichero, &fichero_stat); //rellenamos fichero_stat con el "stat" de fichero
	*n_entradas = fichero_stat.st_size / sizeof(struct utmp);	//n_entradas contiene el numero de structs utmp en el fichero

	*utmp_buf = malloc( sizeof(struct utmp) * (*n_entradas) ); 	//reservamos memoria

	fread(*utmp_buf, sizeof(struct utmp), *n_entradas, fd);	  	//rellenamos *utmp_buf con las entradas de utmp del fichero
	fclose(fd);

	return EXIT_SUCCESS;
}


/* Muestra la lista de usuarios conectados al sistema, segun estan registrados
   en el fichero utmp globalArgs.inputFile.
   Utiliza leer_utmp() para leer el fichero.  */
static void
my_who1 ( void )
{
	int n_entradas; //num de entradas en el fichero de entrada
	struct utmp *utmp_buf;

	if ( leer_utmp( globalArgs.inputFile, &n_entradas, &utmp_buf ) != EXIT_SUCCESS ){
		perror("Error my_who1()");
		exit(EXIT_FAILURE);
	}

	explorar_entradas( n_entradas, utmp_buf );
	free(utmp_buf); //liberamos la memoria reservada para **utmp_buf en leer_utmp()
}


//****************************************************************************//


int main( int argc, char *argv[] )
{
	// inicializar/poner defaults a globalArgs antes de empezar a trabajar
	globalArgs.nombre_programa = argv[0];
	globalArgs.cabecera = false;
	globalArgs.inputFile = UTMP_FILE;
	globalArgs.numInputFiles = 0;

	int opt = 0; // recoge la salida de getopt

	// procesar los args con getopt(), y rellenar globalArgs
	opt = getopt( argc, argv, optString );
	while( opt != -1 ) {
		switch( opt ) {
			case 'H':
				globalArgs.cabecera = true;
				break;

			case 'h':	/* fallthrough , no hay break */
			case '?':
				mostrar_uso();
				break;

			default:
				/* Nunca llega a este caso */
				break;
		}
		opt = getopt( argc, argv, optString );
	}

	// Recoger fichero/s de entrada, si hay
	globalArgs.numInputFiles = argc - optind;
	switch( globalArgs.numInputFiles ) {
		case 0:
			/* Nos quedamos con UTMP_FILE por defecto */
			break;

		case 1:
			globalArgs.inputFile = argv[optind];    // optind: index of the next element to be processed in argv
			break;

		default:
			/* Mas de 1 inputFile, error */
			fprintf(stderr, "Operando/s de mas, a partir de %s\n", argv[optind]);
			exit(EXIT_FAILURE);
			break;
	}


	// Ejecutar el programa
	my_who1();
	exit(EXIT_SUCCESS);

}
