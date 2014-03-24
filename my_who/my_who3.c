#include <stdio.h> 		//para standard I/O
#include <unistd.h>  	//para POSIX API, getopt() getopt tutorial: http://www.ibm.com/developerworks/aix/library/au-unix-getopt.html
#include <stdlib.h> 	//para EXIT_SUCCESS and EXIT_FAILURE  (portabilidad)
#include <utmp.h>		//para el path de utmp y los structs utmp
#include <stdbool.h> 	//ansi C99 estandar (si no, se puede hacer typedef enum { false, true } bool;)
#include <errno.h>		//para errno
#include <sys/stat.h>	//para struct stat
#include <time.h>		//para localtime, strftime
#include <dirent.h>		//para tipo DIR
#include <string.h>		//para strcat
#include <limits.h> 	//para tamaño de short int


#define NOMBRE_UTMP "utmp"  				//nombre del archivo a buscar por defecto
#define DEV_DIR 	"/dev/"					//para comparar con ttyname si es la tty actual
#define DEV_DIR_LEN (sizeof (DEV_DIR) - 1)	//para comparar con ttyname si es la tty actual

extern int errno;  /* Guarda el último valor de salida de llamadas al sistema o
					funciones de librerias. perror() usará ese int para devolver una
					cadena de caracteres informando de que tipo de error es */



/* my_who3
 * soporta los siguientes argumentos:
 *
 * -H incluye una cabecera de introducción a la lista de datos
 * -h muestra un recordatorio de uso de la orden.
 *
 * -l muestra los procesos inicio de sesión "login"
 * -s muestra solo nombre, terminal y fecha de inicio de sesion (por defecto)
 * -u muestra informacion acerca de los usuarios con sesion abierta
 * -m igual que -s pero mostrando sólo el registro correspondiente a la sesión en curso
 *
 * -a todas; la suma de las opciones -b -d -l -r -s.
 * -b registros de arranque (“boot ”)
 * -d registros acerca de procesos “muertos”
 * -q listado de usuarios con sesiones abiertas, indicando su cantidad
 * -r registros sobre selección de nivel de ejecución (run-level )
 *
 * optString le dice a getopt() que opciones de argumentos soportar
 */





// Para guardar las opciones de los argumentos
	struct globalArgs_t {
		char *nombre_programa;
		char *inputFile;					/* input file */
		int numInputFiles;					/* num de ficheros de entrada */
		bool cabecera;						/* opcion -H */
		bool salida_breve;					/* opcion -s */
		bool incluye_usuarios;				/* opcion -u */
		bool muestra_actividad;				/* opcion -u */
		bool incluye_login;					/* opcion -l */
		bool incluye_solo_esta_tty;			/* opcion -m */
		bool incluye_boot;					/* opcion -b */
		bool incluye_dead;					/* opcion -d */
		bool modo_quantity;					/* opcion -q */
		bool incluye_run_level;				/* opcion -r */
											/* opcion -a es -b -d -l -r -s  */
	} globalArgs;

	// Informa a getopt de que opciones se procesan y cuales necesitan un argumento
	static const char *optString = "Hh?lsumabdqr";

// Variables globales
static char const *formato_tiempo = "%b %e %H:%M";   // para la llamada a strftime()
				     //   %b + " " + %e + " " + %H + : +%M + '\0'
static int const anchura_formato_tiempo =  3 +  1  +  2 +  1  +  2 + 1 + 2 +  1;


//****************************************************************************//



/* Mostrar uso de programa y salir */
static void
mostrar_uso( void )
{
	fprintf(stderr, "Uso: %s [ -H | -h |-l | -m |-s |-u] [ fichero o directorio] ...\n", globalArgs.nombre_programa);
	fprintf(stderr, "donde\n");
	fprintf(stderr, "-H \t incluye una cabecera de introducción a la lista de datos\n");
	fprintf(stderr, "-h \t muestra un recordatorio de uso de la orden\n");
	fprintf(stderr, "-l \t muestra los procesos inicio de sesión (\"login\")\n");
	fprintf(stderr, "-s \t Muestra solo nombre, terminal y fecha de inicio de sesion (por defecto)\n");
	fprintf(stderr, "-u \t Muestra informacion acerca de los usuarios con sesion abierta\n");
	fprintf(stderr, "-m \t Igual que -s pero mostrando sólo el registro correspondiente a la sesión en curso\n");
	fprintf(stderr, "-a \t Muestra todas las opciones; la suma de las opciones -b -d -l -r -s\n");
	fprintf(stderr, "-b \t Muestra registros de arranque (“boot ”)\n");
	fprintf(stderr, "-d \t Muestra registros acerca de procesos “muertos”\n");
	fprintf(stderr, "-q \t Muestra listado de usuarios con sesiones abiertas, indicando su cantidad \n");
	fprintf(stderr, "-r \t Muestra registros sobre selección de nivel de ejecución (run-level)\n");
	fprintf(stderr, "\nSi no se especifica FICHERO, usa %s. Es habitual el uso", UTMP_FILE);
	fprintf(stderr, "de %s como FICHERO.\n", WTMP_FILE);
	fprintf(stderr, "Si aparecen ARG1 ARG2, se aplica la opcion -m: `soy yo' es lo usual.\n");
	fflush(stderr); //vacia el buffer a stderr, por si acaso
	exit(EXIT_FAILURE);
}



/* Devuelve un string representando el tiempo transcurrido entre CUANDO y AHORA.
   BOOTTIME es la hora del ultimo arranque.  */
static const char *
string_actividad (time_t cuando, time_t boottime)
{
	/*
	La cadena devuelta es "."  si ha habido actividad en el terminal en el ultimo minuto,
	HH:MM si hace menos de un dia que se ha usado el terminal, 'antig' si hace mas de un dia

	NOTAS
		OJO time_t suele ser un long int
	la funcion difftime(time_t cuando, time_t boottime) devuelve la diferencia de tiempo entre dos time_t
	*/
	time_t dif = difftime (cuando,boottime);

	if( dif < 60 ){ //menos de un minuto
		return ".";
	}else if (dif < (60*60*24) ){ //menos de un dia
			char* timestr = (char *) calloc(8, sizeof(char));
			strftime(timestr, 8, "%H:%M", localtime(&dif));

			return timestr;
		}
	return "antig";
}



/*  Devuelve un string con la fecha/hora.
	hace malloc de timestr, que es devuelto por return
	(acordarse de hacer un free cuando no se utilice)
    return tiempo formateado */
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
	strftime(timestr, anchura_formato_tiempo+1, formato_tiempo, time3);

	return timestr;
}



/* Imprime una linea de salida formateada. */
static void
print_linea (int longusuario, const char *usuario,
	    int longlinea, const char *linea,
	    const char *str_tiempo, const char *actividad,
	    const char *comentario, const char *str_salida)
{
	/* cosas a imprimir:
	 *		NOMBRE 	LINEA 	TIEMPO 	INACT. 	COMENTARIO SALIDA
	 * -l 	X		X		X				X
	 * -s 	X		X		X				X
	 * -u	X		X		X		X		X
	 * -m	X		X		X		X		X
	 * -a	X		X		X				X			X
	 * -b			X		X
	 * -d 			X		X				X			X
	 * -r 			X		X				X
	 *
	 * -q	diferente!
	 *
	 *	vamos a ir una a una checkeando si con las opciones hay que imprimirlas
	*/


	//nombre
	if(!globalArgs.incluye_boot && !globalArgs.incluye_dead && !globalArgs.incluye_run_level)
		printf("%-15.15s\t", usuario);
	//linea
	printf("%-12.12s\t", linea);
	//tiempo
	printf("%-15.14s\t", str_tiempo);
	//inactividad
	if(globalArgs.muestra_actividad)
		printf("%-6.6s\t", actividad);
	//comentario
	if(!globalArgs.incluye_boot)
		printf("%-25.25s\t", comentario);
	//salida
	if(globalArgs.incluye_dead)
		printf("%-20.20s", str_salida);

	printf("\n");
}



static void
print_cabecera (void)
{
	print_linea(15,"NOMBRE",15,"LINEA","TIEMPO","INACT.","COMENTARIO","SALIDA");
}



/*	Construye una cadena "id='campo ut_id' " como comentario
	reserva memoria para construir el comentario
	(acordarse de liberar despues!)
	*/
static char *
id_es_comentario (struct utmp const *utmp_ent)
{
	if (globalArgs.incluye_login || globalArgs.incluye_dead){  //devolvemos: id=ut_id
			char * comentario= calloc( (strlen("id=")+strlen(utmp_ent->ut_id)) , sizeof(char) );	//con malloc no: no inicializa mem
			strcat(comentario, "id=");
			strcat(comentario, utmp_ent->ut_id);
			return comentario;
	}
	else{		//segun enunciado devolvemos host entre parentesis: id=(host)
			char * comentario = calloc( (sizeof(utmp_ent->ut_host) + strlen(")") + strlen("(") ) , sizeof(char) );   //con malloc no: no inicializa mem
			strcat(comentario, "(");
			strcat(comentario, utmp_ent->ut_host);
			strcat(comentario, ")");
			return comentario;
	}
}



/* Envia la informacion referente a un USER_PROCESS, adecuadamente tratada, a print_linea.
   BOOTTIME es el tiempo del ultimo arranque. */
static void
print_usuario (const struct utmp *utmp_ent, time_t boottime)
{
	/* ESQUEMA
		string_actividad() <const char *string_actividad (time_t cuando,time_t boottime) >:
			time()
		string_tiempo() <const char *string_time (const struct utmp *utmp_ent) >:
			localtime()
			strftime()
	*/

	// Recogemos los datos de utmp_ent
	const char *usuario =	utmp_ent->ut_user;
	const char *linea = utmp_ent->ut_line;
	char *tiempo = string_tiempo(utmp_ent); 					// construimos el tiempo
	char *comentario = id_es_comentario(utmp_ent); 					// construimos el comentario
	const char *actividad = string_actividad(time(NULL), boottime); // boottime es el tiempo del ultimo arranque

	print_linea(15, usuario, 15, linea, tiempo, actividad, comentario, ""/*no hay salida*/);
	free(comentario); //de id_es_comentario()
	free(tiempo); //de string_tiempo()

}



static void
print_login (const struct utmp *utmp_ent)
{
	// Recogemos los datos de utmp_ent
	const char* usuario =	utmp_ent->ut_user;
	const char* linea = utmp_ent->ut_line;
	char* tiempo = string_tiempo(utmp_ent);							// construimos el tiempo
	char* comentario = id_es_comentario(utmp_ent); 					// construimos el comentario

	print_linea(15, usuario, 15, linea, tiempo, ""/*no hay actividad*/, comentario,""/*no hay salida*/);
	free(comentario); //de id_es_comentario()
	free(tiempo); //de string_tiempo()
}


static void
print_boot (const struct utmp *utmp_ent)
{
	// Recogemos los datos de utmp_ent
	char* tiempo = string_tiempo(utmp_ent);							// construimos el tiempo

	print_linea(15, ""/*vacio*/, 15, "system boot", tiempo, ""/*no hay actividad*/, ""/*vacio*/,""/*no hay salida*/);
	free(tiempo); //de string_tiempo()

}


static void
print_dead (const struct utmp *utmp_ent)
{
	// Recogemos los datos de utmp_ent
	const char* linea = utmp_ent->ut_line;
	char* tiempo = string_tiempo(utmp_ent);							// construimos el tiempo
	char* comentario = id_es_comentario(utmp_ent); 					// construimos el comentario

	// salida
	short int termination = utmp_ent->ut_exit.e_termination;
	short int exit_var = utmp_ent->ut_exit.e_exit;
	char *aux_salida = calloc  ( ( sizeof("term=") + sizeof("salida=") + SHRT_MAX + SHRT_MAX) , sizeof(char) );
	sprintf(aux_salida,"term=%-2d salida=%-2d",termination, exit_var );

	print_linea(15, ""/*vacio*/, 15, linea, tiempo, ""/*no hay actividad*/, comentario, aux_salida);
	free(comentario); //de id_es_comentario()
	free(tiempo); //de string_tiempo()
	free(aux_salida);
}


static void
print_run_level (const struct utmp *utmp_ent)
{
	// Recogemos los datos de utmp_ent
	char* tiempo = string_tiempo(utmp_ent);							// construimos el tiempo
	char* comentario = id_es_comentario(utmp_ent); 					// construimos el comentario

	print_linea(15, ""/*no hay usuario*/, 15, "run-level", tiempo, ""/*no hay actividad*/, comentario,""/*no hay salida*/);
	free(comentario); //de id_es_comentario()
	free(tiempo); //de string_tiempo()
}


/* Explora el array de registros *utmp_buf, que debería tener n entradas */
static void
explorar_entradas (size_t n, const struct utmp *utmp_buf)
{
	if (globalArgs.cabecera) print_cabecera();

	// Recorremos las entradas
	int num_usuarios = 0;

	int i = 1;
	for (i=1; i<=n; i++){

		short int tipo = utmp_buf[i].ut_type;    //info(que no importa): ejecutando valgrind --tool=memcheck ./my_who3  si es de tipo empty, have invalid read

		if (tipo == 0) printf("\n"); // nos saltamos una vuelta del for para que no escriba nada mas si es empty
		else { // imprimimos informacion solo si no es de tipo 0, es decir, si tiene tipo

			if (globalArgs.incluye_login && (tipo == 6)){
				print_login(&utmp_buf[i]);   // hace printf de una linea de login
			}

			if (globalArgs.incluye_usuarios && (tipo == 7)){
				// creamos nuestra_tty para comparar con la tty de cada linea
				char * nuestra_tty;
				// reservamos memoria dinamicamente por q no sabemos que tty usamos
				nuestra_tty = malloc( (sizeof(utmp_buf[i].ut_line)+ DEV_DIR_LEN ) * sizeof(char));
				// construimos nuestra_tty
				strcat(nuestra_tty, DEV_DIR);
				strcat(nuestra_tty, utmp_buf[i].ut_line);
				// comparamos nuestra_tty con la tty de la linea
				struct stat tty_stat;
				stat(nuestra_tty,&tty_stat);
				if (!globalArgs.incluye_solo_esta_tty) {print_usuario(&utmp_buf[i], tty_stat.st_atime);}		//mostrar los usuarios y usar sus ttys.st_atime
				else 																							//mostrar solo esta sesion
					if (0 == strcmp(nuestra_tty, ttyname(0))){  				//ttyname(0) devuelve la terminal abierta (0 = fd0)
						print_usuario(&utmp_buf[i], tty_stat.st_atime);
					}
				free(nuestra_tty);  //liberamos line despues de usarlo
			}

			if (globalArgs.incluye_boot && (tipo == 2)){
				print_boot(&utmp_buf[i]);   // hace printf de una linea de boot
			}

			if (globalArgs.incluye_dead && (tipo == 8)){
				print_dead(&utmp_buf[i]);   // hace printf de una linea de dead_process
			}

			if (globalArgs.incluye_run_level && (tipo == 1)){
				print_run_level(&utmp_buf[i]);   // hace printf de una linea de run_level
			}

			if (globalArgs.modo_quantity && (tipo == 7)){
				// hemos encontrado un usuario mas
				num_usuarios++;

				// Imprimimos ut_user
				const char *usuario = utmp_buf[i].ut_user;  //ponemos const ya que utmp_buf entra como const a la funcion
				printf("%s", usuario);
				printf(" "); //espacio de separacion
			}

		}//del else de tipo

	}//del for

	if (globalArgs.modo_quantity){
		printf("# usuarios = %d\n",num_usuarios);
	}

}



/* Busca un fichero utmp en el directorio, devolviendo su ruta en ruta_fich_utmp
   devuelve EXIT_SUCESS si lo encuentra, EXIT_FAILURE si no, y lanza errores si
   no puede abrir archivos, etc
 */
static int
buscar_utmp_en_ruta(char *dir, char *ruta_fich_utmp){
	DIR *dirp;  //descriptor del directorio actual
	struct dirent *dp;

	// Abre el directorio dir, y obtiene un descriptor dirp de tipo DIR
	if ((dirp = opendir(dir)) == NULL) {
		perror("Error buscar_utmp_en_ruta()"); return EXIT_FAILURE;
	}

	// Recorremos todas las entradas del directorio
	while ((dp = readdir(dirp)) != NULL) {

		// si encontramos el fichero, lo devolvemos y salimos
		if (strcmp(NOMBRE_UTMP,dp->d_name) == 0) {
			// Construye el nombre del fichero a partir del directorio dir
			ruta_fich_utmp = strcat(dir, dp->d_name);
			closedir(dirp);
			return EXIT_SUCCESS; //encontrado
		}
	}
    closedir(dirp);
	return EXIT_FAILURE; //no encontrado
}



/* Lee el fichero especificado y almcena su contenido
   en un array de STRUCT UTMP
   hace malloc de *utmp_buf, que se devuelve por parametros
   (acordarse de hacer free cuando no se utilice)
   return estado de finalizacion
*/
static int
leer_utmp(char *ruta, int *n_entradas, struct utmp **utmp_buf)
{
	FILE* fd;
	if ((fd = fopen(globalArgs.inputFile, "r")) == NULL) //abrimos el inputFile para lectura
	perror("Error leer_utmp()"), exit(EXIT_FAILURE);

	struct stat ruta_stat;
	stat(ruta, &ruta_stat); //rellenamos ruta_stat con el "stat" de ruta

	// Creamos la ruta:
	// Chequeamos si es fichero regular o directorio
	if (S_ISREG (ruta_stat.st_mode)) {/*vacio*/}//si es regular, ya tenemos la ruta
    	else{
		if (S_ISDIR (ruta_stat.st_mode)) {  //Si es directorio, creamos ruta al fichero utmp
			strcat(ruta,"/"); // para que tire si pones "dir/" o "dir"
				if ( buscar_utmp_en_ruta(ruta,ruta) != EXIT_SUCCESS ){
				fprintf(stderr, "Fichero \"%s\" no encontrado\n",NOMBRE_UTMP);
				exit(EXIT_FAILURE);
			}
			return leer_utmp(ruta,n_entradas, utmp_buf); // chequear recursivamente si el fichero utmp es regular y operar con el
		}
		else{ //el fichero no es ni regular ni directorio
			fprintf(stderr, "El fichero \"%s\" no es de tipo regular o directorio\n",ruta);
			exit(EXIT_FAILURE);
		}
	}

	// Operamos con el fichero
	*n_entradas = ruta_stat.st_size/sizeof(struct utmp);	//n_entradas contiene el numero de structs utmp en el ruta
	*utmp_buf =  malloc( sizeof(struct utmp) * (*n_entradas) ); //reservamos memoria

	fread(*utmp_buf, sizeof(struct utmp), *n_entradas, fd);	  //rellenamos *utmp_buf con las entradas de utmp del ruta
	fclose(fd);

	return EXIT_SUCCESS;
}



/* Muestra la lista de usuarios conectados al sistema,
   Utiliza leer_utmp() para leer el fichero.  */
static void
my_who3 ( void )
{
	int n_entradas; //num de entradas en el fichero de entrada
	struct utmp *utmp_buf;

	if ( leer_utmp( globalArgs.inputFile, &n_entradas, &utmp_buf ) != EXIT_SUCCESS ){
		perror("Error my_who2()");
		exit(EXIT_FAILURE);
	}

	explorar_entradas( n_entradas, utmp_buf );
	free(utmp_buf); //liberamos la memoria reservada para **utmp_buf en leer_utmp()
}



//****************************************************************************//



int main( int argc, char *argv[] )
{
	int opt = 0; // recoge la salida de getopt

	// inicializar/poner defaults a globalArgs antes de empezar a trabajar
	globalArgs.nombre_programa = argv[0];
	globalArgs.cabecera = false;					/* opcion -H */
	globalArgs.inputFile = UTMP_FILE;				/* input file */
	globalArgs.numInputFiles = 0;					/* num de ficheros de entrada */
	globalArgs.incluye_login = false;				/* opcion -l */
	globalArgs.salida_breve = true;					/* opcion -s , por defecto */
	globalArgs.incluye_usuarios = true;				/* opcion -u , por defecto */
	globalArgs.muestra_actividad = false;			/* opcion -u */

	globalArgs.incluye_solo_esta_tty = false;		/* opcion -m */
	globalArgs.incluye_boot = false;				/* opcion -b */
	globalArgs.incluye_dead = false;				/* opcion -d */
	globalArgs.modo_quantity = false;				/* opcion -q */
	globalArgs.incluye_run_level = false;			/* opcion -r */


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

			case 'l':
				globalArgs.incluye_login = true;
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				break;

			case 's':
				globalArgs.salida_breve = true;
				break;

			case 'u':
				globalArgs.salida_breve = false; // deshacer el por defecto
				globalArgs.incluye_usuarios = true;
				globalArgs.muestra_actividad = true;
				break;

			case 'm':
				globalArgs.incluye_solo_esta_tty = true;
				// no deshacemos el por defecto por que tiene q mostrar usuarios
				break;

			case 'a': /* -b -d -l -r -s */
				globalArgs.incluye_boot = true;				/* opcion -b */
				globalArgs.incluye_dead = true;				/* opcion -d */
				globalArgs.incluye_run_level = true;		/* opcion -r */
				globalArgs.incluye_login = true;			/* opcion -l */
				globalArgs.salida_breve = true;		 		/* opcion -s */
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				break;

			case 'r':
				globalArgs.incluye_run_level = true;		/* opcion -r */
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				globalArgs.salida_breve = true;		 // deshacer el por defecto
				break;

			case 'd':
				globalArgs.incluye_dead = true;				/* opcion -d */
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				globalArgs.salida_breve = true;		 // deshacer el por defecto
				break;

			case 'b':
				globalArgs.incluye_boot = true;				/* opcion -b */
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				globalArgs.salida_breve = true;		 // deshacer el por defecto
				break;

			case 'q':
				globalArgs.cabecera = false;
				globalArgs.modo_quantity = true;				/* opcion -q */
				globalArgs.incluye_usuarios = false; // deshacer el por defecto
				globalArgs.salida_breve = true;		 // deshacer el por defecto
				break;

			default:
				/* Nunca llega a este caso */
				break;
		}
		opt = getopt( argc, argv, optString );
	}

	// Recoger fichero de entrada, si hay
	globalArgs.numInputFiles = argc - optind;
	switch( globalArgs.numInputFiles ) {
		case 0:
			/* Nos quedamos con UTMP_FILE por defecto */
			break;

		case 1:
			/* my_who2 [fichero | directorio] */
			globalArgs.inputFile = argv[optind];    // optind: index of the next element to be processed in argv
			break;

		default:
			/* Mas de 1 inputFile, error */
			fprintf(stderr, "Operando/s de mas, a partir de %s\n", argv[optind]);
			exit(EXIT_FAILURE);
			break;
	}

	// Ejecutar el programa
	my_who3();
	exit(EXIT_SUCCESS);
}
