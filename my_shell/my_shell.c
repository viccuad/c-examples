#include <unistd.h>			// lseek(), close(), chdir(), fork(), pipe()
#include <stdlib.h>			// exit(), malloc(), free()
#include <stdio.h>			// getchar(), ungetc(), printf(), stdin
#include <fcntl.h>			// open, O_RDONLY...
#include <string.h>			// strlen(), strcpy(), strchr(), strcmp()
#include <err.h>			// err(), warnx()
#include <sys/wait.h>		// wait()
#include "defs.h"			// BOOL
#include "entorno_lib.h"	// EViniciar(), EVobtener(), EVactualizar()
#include "shell_rutinas.h"	// mostrar_status(), reponersigs(), ignorarsigs()
#include "builtins.h"		// asignar(), exportar(), entorno()



#define MAXARG 20
#define MAXFNAME 500
#define MAXPALABRA 500
#define MAXJOBS 50

static FILE *pilaF[10], *fd_actual, *fperfil, *fguion;
static int npilaF=0;
static int ultimo_estado;

static int trabajos[MAXJOBS];
static int numTrabajos = 0;

typedef enum {T_PALABRA, T_AMP, T_PYC, T_MYRQ, T_MYRQ2, T_MNRQ, T_NL, T_EOF,
				T_AMP2, T_OR, T_OR2} TOKEN ;
static BOOL ejecutar = TRUE; //manda ejecutar el 2º comando
static TOKEN obtener_token(char *palabra);
static TOKEN comando(int *waitpid);
static int invocar(int argc, char *argv[], int fd_ent, char *fichero_ent,
		int fd_sal, char *fichero_sal, BOOL anexo, BOOL background);
static void redirigir(int fd_ent, char *fichero_ent, int fd_sal, char *fichero_sal,
		BOOL anexo, BOOL background);
static int esperar_a(int pid);
static BOOL internos(int argc, char *argv[], int fd_ent, int fd_sal);
static void quitar_job(int pid);

#ifndef TEST_TOKEN
int main(int argc, char *argv[], char *envp[]) {
	char *prompt;
	int pid, fd;
	TOKEN term;
	char perfil[100], *home;

	// Preparar la lectura del fichero de inicialización $HOME/.perfil :
	pilaF[npilaF++]=stdin;														// accede a la pos npilaF == 0, incrementa despues
	home=getenv("HOME"); strcat(perfil, home); strcat(perfil,"/.perfil");
	if (access(perfil, R_OK) == 0) {											// si se puede acceder, se abre para lectura
		fperfil = fopen(perfil, "r");
		pilaF[npilaF++]=fperfil;												// pilaF[0] = stdin, pilaF[1] = fperfil, npilaF == 2
	}

	if ((argc == 2) && (argv[1] != NULL)) {   //si se introduce un argumento con un guion, abrirlo e introducirlo a la pila
		if ((fguion = fopen(argv[1], "r")) == NULL){
			perror("Error fopen() abriendo fichero-guión");
			exit(1);			// salimos ahora, que si no es un engorro abrir la shell para darse cuenta de que no ha hecho el guion
		}
		pilaF[npilaF++] = fguion;
	}
	fd_actual=pilaF[npilaF-1];

	// Ignorar señales provocadas desde el terminal:
	ignorarsigs();

	// Importar las variables de entorno del programa shell_np (obtenidas desde bash) dentro de nuestra shell:
	if (!EViniciar(envp))
		err(1,"No puede inicializar el entorno");

	// Preparar el “prompt” o “solicitud de entrada de orden”:
	if ((prompt = EVobtener("PS2")) == NULL)
		prompt = "> ";
	if (isatty(fileno(fd_actual)))						// es una tty: nos ponemos en modo interactivo
		printf("%s %s", EVobtener("PWD"),prompt);

	/*  CICLO BASICO (repetir continuamente):
		Mostrar el “prompt”, si procede
		Obtener una orden
			Si ya no hay mas, acabar
			Si no, mandarla ejecutar
		Si su ejecución es “foreground, externa”, esperar a que acabe
	*/
	while (TRUE) {
		term = comando(&pid);													// ejecuto el comando, creando hijo si necesario
		if (term != T_AMP && pid != 0)											// si eres el padre, y no hay q hacer background
			ultimo_estado = esperar_a(pid);										// espero a que termine el hijo (ejecutando el comando)
		if (term == T_NL || term == T_EOF)										// si he acabado linea o fichero
			if (isatty(fileno(fd_actual)))										// es una tty: nos ponemos en modo interactivo
				printf("%s %s", EVobtener("PWD"),prompt);						// muestro el prompt
	}
	return 0;
}

#endif /* not TEST_TOKEN   */



static TOKEN obtener_token(char *palabra) {
	enum {NEUTRAL, MAYOR2, COMILLAS, PALABRA,
			AMP2, OR2} estado = NEUTRAL;
	int c;
	char *pal;

	pal = palabra; //fd_actual = stdin; 										//lo dan de entrada, comentarlo para las ampliaciones!
	while ((c = fgetc(fd_actual)) != EOF) {
		switch (estado) {
		case NEUTRAL:
			switch                  (c) {
			case ';': return T_PYC;
			//case '&': return T_AMP;
			case '&': estado = AMP2; continue;
			case '|': estado = OR2; continue;
			case '<': return T_MNRQ;
			case '\n': return T_NL;
			case ' ':
			case '\t': continue;
			case '#': c = '\n';  // si se lee un comentario, limpiar la linea, asi no contendrá nada y será ignorada
			case '>':
				estado = MAYOR2; continue;
			case '"':
				estado = COMILLAS; continue;
			default:
				estado = PALABRA;
				*pal++ = c; continue;
			}
		case MAYOR2:
			if (c == '>') return T_MYRQ2;
			ungetc(c, fd_actual);
			return T_MYRQ;


		case AMP2:
			if (c == '&') return T_AMP2;
			ungetc(c, fd_actual);
			return T_AMP;
		case OR2:
			if (c == '|') return T_OR2;
			ungetc(c, fd_actual);
			return T_OR;


		case COMILLAS:
			switch (c) {
			case '\\':
				*pal++ = getchar(); continue;
			case '"':
				*pal = '\0'; return T_PALABRA;
			default:
				*pal++ = c; continue;
			}
		case PALABRA:
			switch (c) {
			case ';' :
			case '&' :
			case '<' :
			case '>' :
			case '\n' :
			case ' ' :
			case '\t' :
				ungetc(c, fd_actual);
				*pal = '\0'; return T_PALABRA;
			default :
				*pal++ = c; continue;
			}
		}
	}
	return T_EOF;
}



static TOKEN comando(int *p_pid) {
	TOKEN token;
	int argc, fd_ent, fd_sal, pid;
	char *argv[MAXARG+1], fichero_ent[MAXFNAME], fichero_sal[MAXFNAME];
	char palabra[MAXPALABRA];
	BOOL anexo;     															// para concatenar al redirigir entradas
	argc = fd_ent = 0; fd_sal = 1;

	BOOL background;
	anexo = FALSE;
	background = FALSE;



	while (TRUE) {
		switch (token = obtener_token(palabra)) {
		case T_PALABRA:
			/* Acumula un nuevo argumento en el arrray argv[] */

			// copiamos la palabra al argv[]
			if (argc < MAXARG+1){ 												// comprobar que no nos pasamos de num de args
				// Reservamos el espacio para la variable
				argv[argc] = (char*) calloc(sizeof(char), strlen(palabra)+1);	// se hace free al saltar de linea (T_NL)
				strcpy(argv[argc], palabra);
				// Incremento del número de argumentos
				argc++;
			}
			else perror("Error: alcanzado el núm. máximo de argumentos");



			continue;
		case T_MNRQ:
			/* Prepara la redireccion de entrada estándar */

			// leer el siguiente token, que contendra el arg fichero de entrada
			token = obtener_token(fichero_sal);
			// Cambiar el descriptor de salida por el del fichero_sal
			if(token == T_PALABRA ) //de tipo palabra por que es una ruta
				fd_ent = -2;
			else fprintf(stderr,"Error: no puede identificar token fichero_ent");

			continue;
		case T_MYRQ:
		case T_MYRQ2:
			/* Prepara la redireccion de salida estandar */

			if (token== T_MYRQ2) anexo = TRUE;
			// leer el siguiente token, que contendra el arg fichero de salida
			token = obtener_token(fichero_sal);
			// Cambiar el descriptor de salida por el del fichero_sal
			if(token == T_PALABRA ) //de tipo palabra por que es una ruta
				fd_sal = -2;
			else fprintf(stderr,"Error: no puede identificar token fichero_sal");

			continue;
		case T_EOF:
			pilaF[--npilaF];
			if (npilaF == 0)
				exit(0);
			fd_actual=pilaF[npilaF-1];
		case T_AMP:
			background = TRUE;  // recibimos &, se ejecutará en background
		case T_AMP2:
			/* Finaliza la recogida de argumentos */
			argv[argc] = NULL;
			/* Manda ejecutar el comando recopilado y devuelve el pid del proceso mandado ejecutar */
			if (argc > 0)
				pid = invocar(argc,argv,fd_ent,fichero_ent,fd_sal,fichero_sal,anexo,background);
			p_pid = &pid; 														// apuntamos a la dir, para usarlo como un puntero
			if (pid != 0) //error al ejecutar
				ejecutar = FALSE;
			else
			    ejecutar = TRUE;
			argv[0] = NULL;
			argc = 0;

			continue;
		case T_OR2:
		case T_OR:
		case T_PYC:
		case T_NL:
			/* Finaliza la recogida de argumentos */
			/* Manda ejecutar el comando recopilado */
			/* Devuelve el pid del proceso mandado ejecutar
			/* Libera memoria */

			/* Finaliza la recogida de argumentos */
			argv[argc] = NULL;

			/* Manda ejecutar el comando recopilado y devuelve el pid del proceso mandado ejecutar */
			if (argc > 0
				&& ejecutar)
				{
				*p_pid = invocar(argc,argv,fd_ent,fichero_ent,fd_sal,fichero_sal,anexo,background);
				/* Libera memoria */
				// http://stackoverflow.com/questions/14746053/free-all-arguments-in-variadic-function-c
				//argv es un array de strings, por definicion, tiene q acabar en NULL
				int i;
				for(i = 0; i< argc; i++){
					free(argv[i]);												// liberar cada string del array
				}
				argv[0]=NULL;													// por definicion, debe acabar en NULL
				anexo = FALSE;
				ejecutar = TRUE;
			}
			return token;
		}
	}
}



static int invocar(int argc, char *argv[], int fd_ent, char *fichero_ent,
		int fd_sal, char *fichero_sal, BOOL anexo, BOOL background) {
	int pid, i;
	char *s;


	pid = 0;
	if ( internos(argc,argv,fd_ent,fd_sal) == FALSE ){							// si es interno, lo ejecutamos
		// comando externo, creamos un hijo para el nuevo proceso asociado al comando a ejecutar
		pid = fork();

		if (pid == -1)
			perror("Error: internos() fork");
		else if (pid == 0){		/************** código del hijo ***************/

			/* expansion de variables: $var */
			int i;
			for(i = 0; i < argc ; i++) {
				if ( argv[i][0] == '$' ){										// si el comando empieza por $, es variable a expandir
					char *aux = malloc (sizeof(char) * (strlen(argv[i]) -1));
					// extraer la variable de "$variable"
					strcpy(aux, argv[i]+1);									// copiamos $var quitándole "$"
					if(EVobtener(aux) != NULL){
						argv[i] = realloc(argv[i], sizeof(char) *(strlen(EVobtener(aux)) ));
						strcpy(argv[i], EVobtener(aux));
						free(aux);
					}
				}
			}
			// preparar señales para que se puede interrumpir si esta en fg
			if (background == FALSE)
				reponersigs();
			else ignorarsigs();   												//en background no le llegan señales (podría sobrar ya que aún no se ha llamado a reponersigs())
			// preparar entorno
			if (EVactualizar() == FALSE)
				fprintf(stderr, "Error EVactualizar(), no se puede reservar memoria");
			//redirecciones
			redirigir(fd_ent,fichero_ent,fd_sal,fichero_sal,anexo,background);
			//ejecución
			if (execvp(argv[0],argv) == -1)										// ejecuta argv[0]
				perror("Error execvp()");										// si hay error,exec da valor a errno. con perror se ve errno.
		}
		else if (pid>0){		/************** código del padre **************/

			if (background == FALSE){
				//esperar al hijo
				pid = esperar_a(pid);  //DEVUELVE int con el estado de terminacion
			}else{ //en background
				printf(" Ejecutando en background...: proceso %i \n", pid);
					trabajos[numTrabajos] = pid;
					numTrabajos++;
			}

		}
	}

	return pid;

}


static void redirigir(int fd_ent, char *fichero_ent, int fd_sal, char *fichero_sal,
		BOOL anexo, BOOL background) {
	int fd;
	char flags;

	/* Redirigir la entrada y salida estandar, segun los argumentos */

	if(fd_ent != 0) {
		flags = O_RDONLY;
		if ((fd_ent = open(fichero_ent, flags, 0666)) == -1)
			perror("Error redirigir(): Fallo al redirigir la entrada\n");
		if (dup2(fd_ent, 0) == -1)
			perror("Error redirigir(): Fallo al redirigir la entrada\n");
		close(fd_ent);
	}

	if(fd_sal != 1) {
		flags = 0;
		if (!anexo){
			flags = flags | O_CREAT | O_TRUNC;
			flags = flags | O_RDWR;
		}
		else
			flags = flags | O_CREAT | O_APPEND | O_RDWR;
		if ((fd_sal = open(fichero_sal, flags, 0666)) == -1)
			perror("Error redirigir(): Fallo al redirigir la salida\n");
		if (dup2(fd_sal, 1) == -1)
			perror("Error redirigir(): Fallo al redirigir la salida\n");
		close(fd_sal);
	}



}

/**
 * Espera a un hijo dado, pero no se bloquea y sigue mandando informacion de la finalización
 * de los demás hijos que finalicen
 *
 * @param  pid 		pid del hijo a esperar
 * @return         	status del hijo una vez ha acabado, 1 si error
 */
static int esperar_a(int pid) {
	int wpid, status;

	/* Esperar a que finalice el proceso "pid"
	   e informar del estado de terminacion de los que vayan acabando
	 */

	status = 0; 									// para que no sea NULL, y wait almacene informacion de terminacion del hijo en él
	while((wpid = waitpid(pid,&status,0)) >= 0 ){
		mostrar_status(wpid,status);
		if (status == 0)
			quitar_job(pid);
	}

	return status;

}



static void quitar_job(int pid) {
	// quitar un trabajo de la lista de trabajos:
	int i = 0;
	for (i = 0; i < numTrabajos ; i++){						// encontrar la posicion dentro de trabajos[]
		if (trabajos[i] == pid) break;
	}
		printf("pid del trabajo encontrado: $d",trabajos[i]);

	for (i = 0; i < numTrabajos; i++){						// eliminar la pos i, y mover todos a la izquierda
		trabajos[i] = trabajos[i+1];
	}
	numTrabajos--;

}



static BOOL internos(int argc, char *argv[], int fd_ent, int fd_sal) {
	/* Analizar el primer argumento y comprobar si es comando interno
	   Tratarlo en caso afirmativo
	 */

	BOOL salida = TRUE;

	if (strcmp(argv[0],"cd") == 0 ){ 			/* cd [dir] 			*/
		if (argv[1] != NULL){					// se usa con argumentos: cambiamos a dir
			if (chdir(argv[1]) == -1)
				perror("Error internos(): no existe el directorio");
			else{
				char *curr_dir = NULL;
				curr_dir = getcwd(curr_dir,0);  //getcwd hace malloc
				EVasignar("PWD", curr_dir);
				free(curr_dir);
			}
		}
		else { 									// se usa sin argumentos: se cambia al home
			if (chdir(EVobtener("HOME")) == -1)
				perror("Error internos(): el directorio especificado para HOME no existe");
			else EVasignar("PWD", EVobtener("HOME"));
		}
	}
	else if (strcmp(argv[0],"pwd") == 0 ){ 		/* pwd     				*/
		printf("%s\n",EVobtener("PWD"));
	}
	else if (strcmp(argv[0],"set") == 0 ) 		/* set nombre valor     */
		asignar(argc,argv);
	else if (strcmp(argv[0],"export") == 0 ) 	/* export var           */
		exportar(argc, argv);
	else if (strcmp(argv[0],"env") == 0 ){ 		/* env          		*/
		entorno(argc,argv);
	}
	else if (strcmp(argv[0],"exit") == 0 ){ 	/* exit [n] , n salida 	*/
		if (argc = 1) exit(0);
		else if (argc > 2) fprintf(stderr,"Uso: exit [int]\n");
		else exit( atoi(argv[1]) );
	}
	else if (strcmp(argv[0], "source") == 0) {	/* source ruta_fichero  */
		if (argc == 2){
			if ( (fguion = fopen(argv[1], "r")) == NULL )
				perror("Error comando \"source\" abriendo fichero-guión");
			else{
				pilaF[npilaF++] = fguion;
				fd_actual = pilaF[npilaF-1];
			}
		}
		else fprintf(stderr,"Uso: source ruta_fichero\n");
	}
	else if (strcmp(argv[0], "jobs") == 0) {
		if (argc == 1) {
			printf("Trabajos en background (%d):\n", numTrabajos);
			int i;
			for (i = 0; i < numTrabajos; i++)
				printf("\tProceso %d\n", trabajos[i]);
		}
		else
			fprintf(stderr,"Uso: jobs\n");
	}
	else salida = FALSE;						// no se ha reconocido el comando como interno


	return salida;
}


#ifdef TEST_TOKEN
int main(void) {
	char palabra[200];

	while (TRUE)
		switch (obtener_token(palabra)) {
		case T_PALABRA:
			printf("T_PALABRA <%s>\n", palabra); break;
		case T_AMP:
			printf("T_AMP\n"); break;
		case T_PYC:
			printf("T_PYC\n"); break;
		case T_MYRQ:
			printf("T_MYRQ\n"); break;
		case T_MYRQ2:
			printf("T_MYRQ2\n"); break;
		case T_MNRQ:
			printf("T_MNRQ\n"); break;
		case T_NL:
			printf("T_NL\n"); break;
		case T_EOF:
			printf("T_EOF\n");
			exit(0);
		}
}
#endif	/* TEST_TOKEN */




