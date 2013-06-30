/*
 ESTE DOCUMENTO ESTA HECHO PARA VERLO CON TAB SIZE = 4
 */

#include <stdio.h>		// printf()
#include <string.h>		// strsignal()
#include <signal.h>		// sigaction()
#include <err.h>		// err()
#include "defs.h"		// bytebajo(), bytealto(), BOOL
#include <sys/wait.h>	// WIFEXITED,WIFSIGNALED,...

#define MAXSIG	_NSIG

#define BADSIG (void (*)())-1	

static void (*accionint)(), (*accionquit)();	/* Reserva de acciones */ 			//TODO no le veo utilidad


void mostrar_status(int pid, int status) {
	/* CODIGO */
	/* Analiza el status recogido de un wait()
	   y muestra la informacion pertinente 	
	 */
	
	// http://publib.boulder.ibm.com/infocenter/tpfhelp/current/index.jsp?topic=/com.ibm.ztpf-ztpfdf.doc_put.cur/gtpc2/cpp_waitpid.html
	printf("\n");
	if (WIFEXITED(status)) {
		printf("El proceso %d ha finalizado con salida (%d)\n" ,pid,WEXITSTATUS(status));
	}
	else if (WIFSIGNALED(status)) {
		printf("El proceso %d ha finalizado por señal %s\n" ,pid,strsignal(WTERMSIG(status)));
	}
	else if (WIFSTOPPED(status)) {     
		printf("El proceso %d ha sido detenido mediante señal %s\n" ,pid,strsignal(WSTOPSIG(status)));
	}
	else if (WIFCONTINUED(status)) {     
		printf("El proceso %d ha sido reanudado mediante señal SIGCONT\n" ,pid);
	}
	else if (WCOREDUMP(status)) {     
		printf("El proceso %d ha generado un volcado de memoria\n" ,pid);
	}

}	



void mi_sighandler(int signum){
    printf("Recibida señal %s\n", strsignal(signum));
}




void ignorarsigs(void) { 

	/* Si es la primera vez guarda las acciones asociadas a SIGINT y SIGQUIT 		//TODO no le veo utilidad a guardar las acciones la 1º vez
	   En todo caso, ignora SIGINT y SIGQUIT, a partir de ahora
	 */
	
	/* 
	manual gnu glibc: https://www.gnu.org/software/libc/manual/html_node/Signal-Handling.html
	manual señales con sa_flags: http://www.alexonlinux.com/signal-handling-in-linux

	formato de un struct sigaction, que se le pasa a la funcion sigaction():
	 
	struct sigaction {
	              void     (*sa_handler)(int);    						Pointer to a function which specifies the action to be associated with signum or one of the macros SIG_IGN or SIG_DFL
	              void     (*sa_sigaction)(int, siginfo_t *, void *);   If SA_SIGINFO is in flags, this is the sig handler function
	              sigset_t   sa_mask;									Additional set of signals to be blocked during execution of signal-catching function.
	              int        sa_flags;									Special flags to affect behavior of signal. eg: SA_SIGINFO
	              void     (*sa_restorer)(void); 						DEPRECATED IN POSIX 
	          };
	
	*/

	struct sigaction sa_sigint;

	sa_sigint.sa_handler = mi_sighandler;//accionint;  	// ponemos nuestra funcion handler 			// TODO accionint(): no le veo utilidad
	sigemptyset(&sa_sigint.sa_mask);					// resetamos el set de señales adicionales que manejar
    sa_sigint.sa_flags = SA_RESTART; 					// vuelve a llamar a funciones del sistema (open, close..) si han sido interrumpidas por nuestro handler
    
    // Ponemos nuestro handler para SIGINT
    if (sigaction(SIGINT, &sa_sigint, NULL) == -1)
       perror("Error ignorarsigs() SIGINT");
    

    struct sigaction sa_sigquit;

	sa_sigquit.sa_handler = mi_sighandler;//accionint;  // ponemos nuestra funcion handler     		// TODO accionint(): no le veo utilidad
    sigemptyset(&sa_sigquit.sa_mask);					// resetamos el set de señales adicionales que manejar
    sa_sigquit.sa_flags = SA_RESTART; 					// vuelve a llamar a funciones del sistema (open, close..) si han sido interrumpidas por nuestro handler

	// Ponemos nuestro handler para SIGQUIT
    if (sigaction(SIGQUIT, &sa_sigquit, NULL) == -1)
       perror("Error ignorarsigs() SIGQUIT");

}

void reponersigs(void) {

	/* Recupera las acciones iniciales de SIGINT y SIGQUIT */

	struct sigaction sa_default;

	sa_default.sa_handler = SIG_DFL;  					// SIG_DFL = sig handler default definido en la libreria glibc
    sigemptyset(&sa_default.sa_mask);					// resetamos el set de señales adicionales que manejar
    sa_default.sa_flags = SA_RESTART; 					// vuelve a llamar a funciones del sistema (open, close..) si han sido interrumpidas por nuestro handler


	// Reponemos handler default para SIGINT
    if (sigaction(SIGINT, &sa_default, NULL) == -1)
       perror("Error reponersigs SIGINT");
      
	// Reponemos handler default para SIGQUIT
    if (sigaction(SIGQUIT, &sa_default, NULL) == -1) 
       perror("Error reponersigs SIGQUIT");
   	

}

	
