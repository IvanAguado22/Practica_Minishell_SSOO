//  MSH Main File

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// Ficheros por si hay redirección
char filev[3][64];

// Para guardar el segundo parámetro de execvp
char *argv_execvp[8];

void siginthandler(int param) {
	printf("\n**** Saliendo del MSH ****\n");
	//signal(SIGINT, siginthandler);
        exit(0);
}

/**
 * Obtener el comando cuyos parámetros son para execvp
 * Ejecutar esta instrucción antes de correr un execvp para obtener el comando completo
 * @param argvv
 * @param num_command
 * @return
 */

void getCompleteCommand(char ***argvv, int num_command) {
    // Reset first
    for (int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

/**
 * Bucle del main shell
 */

int main(int argc, char* argv[]) {
    /**** Do not delete this code.****/
    int end = 0;
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];
    const char *name = "Acc"; // Nombre variable de entorno
    const char *value = "0"; // Valor inicial variable de entorno
    int overwrite = 1; // Valor de sobreescritura
    setenv(name, value, overwrite); // Creamos la variable de entorno
    char textAux[1000];

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF) {
            if (strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;

	while (1) {
		int status = 0;
	        int command_counter = 0;
		int in_background =0;
		signal(SIGINT, siginthandler);

		// Prompt
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
                //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
                executed_cmd_lines++;
                if( end != 0 && executed_cmd_lines < end) {
                    command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
                }else if( end != 0 && executed_cmd_lines == end)
                    return 0;
                else
                    command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
                //************************************************************************************************

                /************************ STUDENTS CODE ********************************/

		if (command_counter > 0) {
			//Si se supera el num max de comandos imprimo el error y salgo del proceso.
			if (command_counter > MAX_COMMANDS) {
				printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
				exit(-1);
			}

		else {
			int fd[2]; // Variable para el pipe
			int pid;
			// Guardo las variables std
			int stdin = dup(STDIN_FILENO);
			int stdout = dup(STDOUT_FILENO);
			int errnum = dup(STDERR_FILENO);

		// Bucle para leer un numero n de comandos
		for (int i = 0; i < command_counter; i++) {

			int pipeControler = pipe(fd); // Creo la tuberia

			// Control de error en el pipe
			if (pipeControler < 0) {
				fprintf(stderr, "Error en el pipe: %s \n", strerror(errnum));
				exit(-1);
				return -1;
			}

			// Si me encuentro en el ultimo hijo
			if (i == command_counter-1) {
				close(fd[1]);
			}

			else {
				dup2(fd[1], STDOUT_FILENO); // Meto la salida en la variable std
				close(fd[1]);
			}

			if (strcmp(argvv[i][0],"mycalc") == 0) { // Primer mandato interno denominado "mycalc"
				int numOps = 0; // Compruebo el número de operandos
				for (int x = 0; x < 8; x++) {
					if (argvv[i][x] == NULL) break; // Condición de salida del bucle
					numOps += 1; // Aumento en uno el número
				}

				if (numOps == 4) { // Debe de tener cuatro componentes
					if (strcmp(argvv[i][2], "add") == 0) { // Condición de suma
						int op1 = atoi(argvv[i][1]); // Conversión a entero op1
 						int op2 = atoi(argvv[i][3]); // Conversión a entero op2
						int sum = op1 + op2;
						char *accAux = getenv(name); // Guardamos el valor actual de la variable de entorno
						int intAccAux = atoi(accAux); // Conversión a entero
						int sumAux = sum + intAccAux; // Actualización de Acc
						sprintf(textAux, "%d", sumAux); // Conversión a String
						setenv(name, textAux, overwrite); // Actualizamos de la variable de entorno
						// Imprimimos el resultado por stderr
						fprintf(stderr, "[OK] %d + %d = %d; Acc %s \n", op1, op2, sum, textAux);
						break;
					}

                			if (strcmp(argvv[i][2], "mod") == 0) { // Condición de módulo
						int quo = 0;
						int rem = 0;
						int op1 = atoi(argvv[i][1]);
						int op2 = atoi(argvv[i][3]);
						quo = op1 / op2;
						rem = op1 % op2;
						// Imprimimos el resultado por stderr
						fprintf(stderr, "[OK] %d %s %d = %d * %d + %d \n", op1, "%", op2, op2, quo, rem);
						break;
					}
					// Si no respeta la operación de suma o módulo imprimirmos un error
					printf("%s", "[ERROR] La estructura del comando es <operando 1> <add/mod> <operando 2> \n");
					break;
				}
				// Si no tiene cuatro componentes imprimimos un mensaje de error
				printf("%s", "[ERROR] La estructura del comando es <operando 1> <add/mod> <operando 2> \n");
				break;
			}

			if (strcmp(argvv[i][0],"mycp") == 0) { // Segundo mandato interno denominado "mycp"
				int numOps = 0; // Compruebo el número de operandos
				for (int x = 0; x < 8; x++) {
					if (argvv[i][x] == NULL) break; // Condición de salida del bucle
					numOps += 1; // Aumento en uno el número
				}

				if (numOps == 3) { // Debe de tener tres componentes
					int fdOrigen = open(argvv[i][1], O_RDONLY); // Guardamos el valor del fichero origen
					int fdDestino = open(argvv[i][2], O_WRONLY | O_CREAT, 0666); // Guardamos el valor del fichero destino

                			if (fdOrigen == -1) {
                        			printf("%s", "[ERROR] Error al abrir el fichero origen \n"); // Error al abrir el fichero origen
                        			break;
                			}

					if (fdDestino == -1) {
						printf("%s", "[ERROR] Error al abrir el fichero destino \n"); // Error al abrir el fichero destino
                        			break;
					}

                			char buf[1000]; // Variable para guardar la información que leamos
                			ssize_t bytes_leidos; // Variable para saber si se han leído todos los bytes y/o saber cuántos

					do {
                        			bytes_leidos = read(fdOrigen, buf, 1000); // Guardamos el valor y leemos del descriptor para guardar en buf
                        			if (bytes_leidos == -1) {
                                			printf("Error de lectura \n"); // Error al leer el fichero
                               				break;
                       				}
                       				write(fdDestino, buf, bytes_leidos); // Escribimos en el fichero destino
               				} while (bytes_leidos != 0); // Condición de bucle para seguir copiando

                			fdOrigen = close(fdOrigen); // Cerramos el fichero Origen
					fdDestino = close(fdDestino); // Cerramos el fichero Destino

					if (fdOrigen == -1) {
                        			printf("Error de cierre del fichero Origen \n"); // Error al cerrar el fichero Origen
                        			break;
                			}

					if (fdDestino == -1) {
                        			printf("Error de cierre del fichero Destino \n"); // Error al cerrar el fichero Destino
                        			break;
                			}
					printf("%s %s %s %s %s", "[OK] Copiado con exito el fichero ", argvv[i][1], " a ", argvv[i][2], " \n");
					break;
				}
				// Si no tiene tres componentes imprimimos un mensaje de error
				printf("%s", "[ERROR] La estructura del comando es mycp <fichero origen> <fichero destino> \n");
				break;
			}

			pid = fork();

			switch (pid) {

				// Control de error en el fork
				case -1:
				perror("Error en el fork \n");
				exit(-1);

				// Hijo
				case 0:
				// Compruebo si hay una redireccion de entrada y ademas estamos en el primer mandato
				// En caso de que si abro el fichero y almaceno su fd en la entrada std
				if (strcmp(filev[0], "0") != 0 && i == 0) {
					close (STDIN_FILENO);
					open(filev[0], O_RDONLY);
				}

				// Compruebo si hay una redireccion de salida y ademas estamos en el ultimo mandato
				// En caso de que si abro el fichero y almaceno su fd en la salida std
				if (strcmp(filev[1], "0") != 0 && i == command_counter-1) {
					close (STDOUT_FILENO);
					open(filev[1], O_CREAT | O_WRONLY, 0666);
				}

				// Compruebo si hay una redireccion de salida error
				// En caso de que si abro el fichero y almaceno su fd en la salida error std
				if (strcmp(filev[2], "0") != 0) {
					close (STDERR_FILENO);
					open(filev[2], O_CREAT | O_WRONLY, 0666);
				}

				close(fd[0]);

				// Realizo el exec y lo almaceno en una variable para posteriormente poder controlar un posible error
				int execControler =  execvp(argvv[i][0], argvv[i]);

				// Control de error en el exec
				if (execControler < 0) {
					fprintf(stderr, "Error en el exec: %s \n", strerror(errnum));
					exit(-1);
					return -1;
				}

				break;

				// Padre
				default:
				// En caso de ser un mandato en background el padre no realiza el wait()
				if (in_background == 1) {
					dup2(fd[0], STDIN_FILENO);
					dup2(stdout, STDOUT_FILENO);
					close(fd[0]);
				}

				else {
					int waitControler = wait(&status); // Realizo el wait para esperar la finalizacion del hijo
					// Lo almaceno en una variable para posteriormente poder controlar un posible error

					// Control de error en el wait
					if (waitControler < 0) {
						fprintf(stderr, "Error en el wait: %s \n", strerror(errnum));
						exit(-1);
						return -1;
				}

				// Guardo la entrada del hijo actual en la entrada std para que la recoja el siguiente hijo
				dup2(fd[0], STDIN_FILENO);
				dup2(stdout, STDOUT_FILENO); // Restauro salida std
				close(fd[0]);
				}
			}
		}
		dup2(stdin, STDIN_FILENO); // Restauro entrada std
		close(stdin);
		close(stdout);
		}
	  }
   }
	return 0;
}
