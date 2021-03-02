#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>


#define VERSION		24
#define BUFSIZE		8096
#define ERROR		42
#define LOG			44
#define OK	200
#define PROHIBIDO	403
#define NOENCONTRADO	404
#define BAD_REQUEST	400
#define NOT_ALLOWED	405
#define UNSUPPORTED_MEDIA 415
#define NOT_IMPLEMENTED 501
#define END_CHAR	'\0'	
#define GET_METHOD	1
#define POST_METHOD	2
#define ALIVE 10
#define CLOSE 11
#define STATUS_OK 55
#define STATUS_ERROR 56
#define EMAIL "gonzalo.nicolasm%40um.es"


struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpg" },
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"ico", "image/ico" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{0,0} };

void debug(int log_message_type, char *message, char *additional_info, int socket_fd)
{
	//(void)printf("Entra al debug");
	//(void)printf("ARGS: %d, %s, %s, %d", log_message_type, message, additional_info, socket_fd);
	int fd ;
	char logbuffer[BUFSIZE*2];
	
	switch (log_message_type) {
		case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",message, additional_info, errno,getpid());
			break;
		case PROHIBIDO:
			// Enviar como respuesta 403 Forbidden
			(void)sprintf(logbuffer,"FORBIDDEN: %s:%s",message, additional_info);
			break;
		case NOENCONTRADO:
			// Enviar como respuesta 404 Not Found
			(void)sprintf(logbuffer,"NOT FOUND: %s:%s",message, additional_info);
			break;
		case BAD_REQUEST:
			(void)sprintf(logbuffer,"BAD REQUEST: %s:%s",message, additional_info);
			break;
		case NOT_IMPLEMENTED:
			(void)sprintf(logbuffer,"NOT IMPLEMENTED: %s:%s",message, additional_info);
			break;
		case UNSUPPORTED_MEDIA:
			(void)sprintf(logbuffer,"UNSUPPORTED MEDIA: %s:%s",message, additional_info);
			break;
		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",message, additional_info, socket_fd); break;
	}



	if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
		//(void)printf("Abre el webserver.log");
	}


	/*if(log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO
		|| log_message_type == BAD_REQUEST || log_message_type == NOT_IMPLEMENTED 
		|| log_message_type == UNSUPPORTED_MEDIA) exit(3);*/
}

// Función para parsear que tipo de método es la peticion HTTP recibida.

int parse_method(char * metodo) {
	if (metodo == NULL) {
		return -2;
	} else if (strcmp(metodo, "GET") != 0 && strcmp(metodo, "POST") != 0) {
		return -1;
	} else if (strcmp(metodo, "POST") == 0){
		return POST_METHOD;
	} else return GET_METHOD;
}


int parse_path(char * path) {
	if (path == NULL){
		return -2;
	}
	else if (strcmp(path, "/") == 0) {	// Me esta haciendo una peticion de tipo host/
		return 1;
	} else {
		for (int i = 0; i < strlen(path) -1; i++) {	// Compruebo si el path es erroneo (intenta acceder a directorios superiores)
			if (path[i] == '.' && path[i+1] == '.') return -1;
		}
		return 2;	// El path es correcto.
	}
}

int parse_extension(char * path) {
	char * ext = strchr(path, '.');		// +1 ?
	printf("\nExtension: %s\n", ext);
	debug(LOG, "Extension", ext, 4);
	if (ext == NULL) {		// Fichero sin extension
		return -2;
	} else {		// Comprobamos si es una extension aceptada (struct extensions)
		for (int i = 0; extensions[i].ext != 0; i++){
			if (strcmp(ext+1, extensions[i].ext) == 0){
				return i;	// Devuelvo el indice de la extension correspondiente
			}
		}
		return -1;		// Extension no aceptada
	}
}

void respuesta(int fd, int file, int codigo, int nExtension) {

	char respuesta[BUFSIZE] = {0};

	// Linea de estado

	strcat(respuesta, "HTTP/1.1 ");

	switch (codigo)
	{
	case OK:
		strcat(respuesta, "200 OK\r\n");
		break;
	case PROHIBIDO:
		strcat(respuesta, "403 FORBIDDEN\r\n");
		break;
	case NOENCONTRADO:
		strcat(respuesta, "404 NOT FOUND\r\n");
		break;
	case NOT_ALLOWED:
		strcat(respuesta, "405 NOT ALLOWED\r\n");
		break;
	case UNSUPPORTED_MEDIA:
		strcat(respuesta, "415 UNSUPPORTED MEDIA\r\n");
		break;
	case NOT_IMPLEMENTED:
		strcat(respuesta, "501 NOT IMPLEMENTED\r\n");
		break;
	case BAD_REQUEST:
		strcat(respuesta, "400 BAD REQUEST\r\n");
		break;
	}

	//
	// CABECERA
	//

	//	Linea Date

	struct tm gtime;
	time_t now;
	time(&now);
	gtime = *gmtime(&now);

	strcat(respuesta, "Date: ");

	char aux[256] = {0};

	strftime(aux, sizeof aux, "%a, %d %b %Y %H:%M:%S %Z", &gtime);
	strcat(respuesta, aux);
	strcat(respuesta, "\r\n");

	//	Linea Server
	strcat(respuesta, "Server: sstt2021\r\n");

	//	Last modified - ETag - Accept-Ranges (?)

	//	Linea Content-Length
	struct stat filestat;
	
	if (file != -1) {	// Se ha podido abrir un fichero
		fstat(file, &filestat);
		memset(aux, 0, sizeof aux);	
		sprintf(aux, "Content-Length: %ld\r\n", filestat.st_size);
		strcat(respuesta, aux);
	} else {	// No se ha podido abrir un fichero
		strcat(respuesta, "Content-Length: 0\r\n");
	}

	//	Linea Keep-Alive
	strcat(respuesta, "Keep-Alive: timeout=10, max=0\r\n");
	strcat(respuesta, "Connection: Keep-Alive\r\n");

	//	Linea Content-Type
	if (nExtension < 0) {
		strcat(respuesta, "Content-Type: \r\n");	
	} else {
		strcat(respuesta, "Content-Type: ");
		strcat(respuesta, extensions[nExtension].filetype);
		strcat(respuesta, "; charset=ISO-8859-1\r\n");
	}

	strcat(respuesta, "\r\n");

	write(fd, respuesta, strlen(respuesta));

	//
	//	CONTENIDO SOLICITADO - DATOS
	//

	// Si hay fichero abierto, empiezo a escribir.

	if (file != -1) {
		char buffer[BUFSIZE];	// Buffer de 8k
		memset(buffer, 0, sizeof buffer);		// Preparo el buffer

		ssize_t readsize;
		while(readsize = read(file, buffer, BUFSIZE)) {
			write(fd, buffer, BUFSIZE);
		}


	}




}

void process_web_request(int descriptorFichero)
{
	debug(LOG,"request","Ha llegado una peticion",descriptorFichero);
	//
	// Definir buffer y variables necesarias para leer las peticiones
	//

	
	fd_set fdset;	// Conjunto de descriptores de ficheros.
	struct timeval tv;		// Timeout
	int retval;		// Valor devuelto por select()
	int persistencia = ALIVE;	// Variable de persistencia. Por defecto, se mantiene la conexion abierta
	int status = STATUS_OK;		// Variable de estado. Sirve para indicar si se ha producido un error y no mandar multiples respuestas.
	

	// Queremos trabajar con el fd  "descriptorFichero"
	// Vacio el conjunto de descriptores de fichero
	// Añado el descriptor de fichero al conjunto

	FD_ZERO(&fdset); 
	FD_SET(descriptorFichero, &fdset);

	// Espero hasta 5 segundos (Siguiendo el ejemplo de man 2 select)

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	retval = select(descriptorFichero+1, &fdset, NULL, NULL, &tv);

	// Si en el readfds hay caracteres disponibles (1 = readfds = lectura)
	// readfds corresponde en este caso a fdset, el cual contiene descriptorFichero
	// Procedo a manejar dichos caracteres disponibles

	// Tengo que hacer un loop para que pueda seguir leyendo despues de \n\r

	while (retval) {

		// Inicio memoria de buffer todo a ceros, y preparo una variable
		// para saber cuanto he leido del descriptor de fichero.

		char buffer[BUFSIZE] = {0};	// Buffer	
		//
		// Leer la petición HTTP
		//
		ssize_t readsize = read(descriptorFichero, buffer, BUFSIZE);
		debug(LOG, "Cadena recibida", buffer, descriptorFichero);
		
		
		//
		// Comprobación de errores de lectura
		//

		if (readsize < 0) {
			debug(ERROR, "Error en la lectura", "Ha ocurrido un error al leer el buffer ", descriptorFichero);
			close(descriptorFichero);
		} 
		
		//
		// Si la lectura tiene datos válidos terminar el buffer con un \0
		//
		
		buffer[readsize] = END_CHAR;
		
		//
		// Se eliminan los caracteres de retorno de carro y nueva linea
		//
		
		// Y si en vez de eliminarlos, extraigo elementos utilizando \r\n como separador?
		
		char * request_line;	// Linea de peticion HTTP
		char * save_ptrl;		// Usado para mantener la posicion en strtok_r() del mensaje completo.

		request_line = strtok_r(buffer, "\r\n", &save_ptrl);

		char * metodo;	// Método (GET/POST)
		char * path;	// Path del objeto que se pide
		char * protocolo;	// Protocol HTTP

		metodo = strtok(request_line, " ");
		path = strtok(NULL, " ");
		protocolo = strtok(NULL, "");

		char * header_line;		// Primera linea de cabecera HTTP
		char * host;			// Cadena correspondiente a "Host:""
		char * server;			// Cadena correspondiente a la direccion del servidor
		header_line = strtok_r(NULL, "\r\n", &save_ptrl);	// Primera linea de cabecera

		if (header_line != NULL) {		// Compruebo si hay cabecera 

			host = strtok(header_line, " ");
			server = strtok(NULL, "");

			if (host == NULL || server == NULL) {
				respuesta(descriptorFichero, -1, BAD_REQUEST, -1);
				status = STATUS_ERROR;
				debug(ERROR, "Header error", "Cabecera mal formada", descriptorFichero);
			}

		} else {
			// Generar respuesta?
			respuesta(descriptorFichero, -1, BAD_REQUEST, -1);
			status = STATUS_ERROR;
			debug(ERROR, "Header error", "No existe cabecera HTTP", descriptorFichero);
			//close(descriptorFichero);	// ?

		}

		char * connection;
		char * connection_type;
		int found_connection = 0;

		while(!found_connection && (header_line = strtok_r(NULL, "\r\n", &save_ptrl)) != NULL) {
			connection = strtok(header_line, " ");

			if (strcmp(connection, "Connection:") == 0) {
				connection_type = strtok(NULL, "");
				if (strcmp(connection_type, "close") == 0) {
					persistencia = CLOSE;
				}
				else if (strcmp(connection_type, "keep-alive") == 0){
					persistencia = ALIVE;
				} 

				//debug(LOG, connection, connection_type, descriptorFichero);
				found_connection = 1;
			}
		}


		
		//
		//	TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
		//	(Se soporta solo GET)
		//


		// Dependiendo de que código me devuelva la funcion, hago algo
		// Compruebo los casos de ERROR. Mas facil manejar

		// Si no hay ningun error, continuo analizando.
		// Nota: metodo POST todavia no implementado.

		int method_code = parse_method(metodo);
		if(status == STATUS_OK){
			switch (method_code)
			{
				case -1:
					// Generar respuesta con codigo: NOT_IMPLEMENTED (501)
					respuesta(descriptorFichero, -1, NOT_IMPLEMENTED, -1);
					status = STATUS_ERROR;
					debug(NOT_IMPLEMENTED, "Method error", metodo, descriptorFichero);				
					break;
				
				case -2:
					// Generar respuesta con codigo: BAD_REQUEST (400)
					respuesta(descriptorFichero, -1, BAD_REQUEST, -1);
					status = STATUS_ERROR;
					debug(BAD_REQUEST, "Method error", metodo, descriptorFichero);
					break;
				
				default:
					//debug(LOG, "Metodo GET", metodo, descriptorFichero);
					break;
			}
		}

		
		//
		//	Como se trata el caso de acceso ilegal a directorios superiores de la
		//	jerarquia de directorios
		//	del sistema
		//

		// Dependiendo de que código me devuelva la funcion, hago algo
		// Compruebo los casos de ERROR. Mas facil manejar.
		
		int path_code = parse_path(path);
		if(status == STATUS_OK) {
			switch (path_code)
			{
				case -2:
					// Generar respuesta con codigo: BAD_REQUEST (400)
					respuesta(descriptorFichero, -1, BAD_REQUEST, -1);
					status = STATUS_ERROR;
					debug(BAD_REQUEST, "Path error", path, descriptorFichero);
					break;
				case -1:
					// Generar respuesta con codigo: FORBIDDEN (403)
					respuesta(descriptorFichero, -1, PROHIBIDO, -1);
					status = STATUS_ERROR;
					debug(PROHIBIDO, "Path error", path, descriptorFichero);
					break;
				
				default:
					//debug(LOG, "Path aceptado", path, descriptorFichero);
					break;
			}

		}


		// Dependiendo del path code, se tratara de una forma u otra a continuacion
		// path code = 1 --> host/index.html
		// path code = 2 --> host/fichero
		
		char filepath[PATH_MAX] = {0};
		int nExtension;

		// Caso peticion tipo "/"

		if (strcmp(metodo, "GET") == 0) {
			if (path_code == 1 && status == STATUS_OK) {
				strcat(filepath, "index.html");

				int file = open(filepath, O_RDONLY);

				if (file != -1) {
					// Generar respuesta con codigo: OK (200)
					respuesta(descriptorFichero, file, OK, 9);	// ext 9 = text/html
					//debug(LOG, "Generado respuesta", "index.html", descriptorFichero);
				} else {
					// Generar respuesta con codigo: NOT_FOUND (404)
					respuesta(descriptorFichero, -1, NOENCONTRADO, -1);
					status = STATUS_ERROR;
					debug(NOENCONTRADO, "No se ha encontrado el fichero", "index.html", descriptorFichero);
				}
				close(file);

			}
			else if (path_code > 1 && status == STATUS_OK) {	// Otro caso

				// Tengo que comprobar la extension
				nExtension = parse_extension(path);
				switch (nExtension)
				{
				case -2:
					// Generar respuesta con codigo: UNSUPPORTED_MEDIA (415) -- O hacer un BAD_REQUEST(400) ?
					respuesta(descriptorFichero, -1, UNSUPPORTED_MEDIA, -1);
					status = STATUS_ERROR;
					debug(UNSUPPORTED_MEDIA, "Fichero sin extension", path, descriptorFichero);
					break;
				case -1:
					// Generar respuesta con codigo: UNSUPPORTED_MEDIA (415) - Quizas 406?
					respuesta(descriptorFichero, -1, UNSUPPORTED_MEDIA, -1);
					status = STATUS_ERROR;
					debug(UNSUPPORTED_MEDIA, "Fichero con extension no soportada", path, descriptorFichero);
					break;
				default:
					break;
				}

				strcpy(filepath, path + 1);

				//  |	Salta "/" inicial del path
				//  v
				// /path/......

				int file = open(filepath, O_RDONLY);

				if (file != -1 && status == STATUS_OK) {
					// Generar respuesta con codigo: OK (200)
					respuesta(descriptorFichero, file, NOT_IMPLEMENTED, nExtension);
					//debug(LOG, "Generado respuesta", filepath, descriptorFichero);
				} else if (file < 0 && status == STATUS_OK){
					// Generar respuesta con codigo: NOT_FOUND (404)
					respuesta(descriptorFichero, -1, NOENCONTRADO, nExtension);
					status = STATUS_ERROR;
					debug(NOENCONTRADO, "No se ha encontrado el fichero", filepath, descriptorFichero);
				}
				close(file);
			}
		} else if (strcmp(metodo, "POST") == 0) {
			char * email;
			char * content_line;
			int found_email = 0;

			while(!found_email && (content_line = strtok_r(NULL, "\r\n", &save_ptrl)) != NULL) {
				email = strtok(content_line, "=");
				if (strcmp(email, "email") == 0) {
					email = strtok(NULL, "\n\r");
					found_email = 1;
					if (email == NULL) {
						respuesta(descriptorFichero, -1, BAD_REQUEST, -1);
						status = STATUS_ERROR;
						debug(BAD_REQUEST, "No se ha enviado una direccion email", email, descriptorFichero);
					}
				}
			}


			if (status == STATUS_OK && email != NULL) {
				if (strcmp(email, EMAIL) == 0) {
					debug(LOG, "Email correcto recibido", email, descriptorFichero);
				} else {
					debug(LOG, "Email erroneo recibido", email, descriptorFichero);
				}
			}

		}




		//
		//	PERSISTENCIA
		//

		if (persistencia == ALIVE) {
			FD_ZERO(&fdset); 
			FD_SET(descriptorFichero, &fdset);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			status = STATUS_OK;
			retval = select(descriptorFichero+1, &fdset, NULL, NULL, &tv);
			printf("\nRETVAL: %d", retval);
		} else {
			retval = 0;
		}

		

		
		
		//
		//	Como se trata el caso excepcional de la URL que no apunta a ningún fichero
		//	html
		//

		
		
		
		//
		//	Evaluar el tipo de fichero que se está solicitando, y actuar en
		//	consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
		//
		
		
		//
		//	En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
		//	correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
		//

	}
	printf("\nSaliendo..");
	close(descriptorFichero);
	exit(1);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr;		// static = Inicializado con ceros
	static struct sockaddr_in serv_addr;	// static = Inicializado con ceros
	
	//  Argumentos que se esperan:
	//
	//	argv[1]
	//	En el primer argumento del programa se espera el puerto en el que el servidor escuchara
	//
	//  argv[2]
	//  En el segundo argumento del programa se espera el directorio en el que se encuentran los ficheros del servidor
	//
	//  Verficiar que los argumentos que se pasan al iniciar el programa son los esperados
	//

	//
	//  Verficiar que el directorio escogido es apto. Que no es un directorio del sistema y que se tienen
	//  permisos para ser usado
	//

	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: No se puede cambiar de directorio %s\n",argv[2]);
		exit(4);
	}
	// Hacemos que el proceso sea un demonio sin hijos zombies
	if(fork() != 0){
		//(void)printf("Entra al fork");
		return 0; // El proceso padre devuelve un OK al shell
	}
	
	//(void)printf("No Entra al fork");

	(void)signal(SIGCHLD, SIG_IGN); // Ignoramos a los hijos
	(void)signal(SIGHUP, SIG_IGN); // Ignoramos cuelgues
	
	debug(LOG,"web server starting...", argv[1] ,getpid());
	
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		debug(ERROR, "system call","socket",0);
	
	port = atoi(argv[1]);
	
	if(port < 0 || port >60000)
		debug(ERROR,"Puerto invalido, prueba un puerto de 1 a 60000",argv[1],0);
	
	/*Se crea una estructura para la información IP y puerto donde escucha el servidor*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Escucha en cualquier IP disponible*/
	serv_addr.sin_port = htons(port); /*... en el puerto port especificado como parámetro*/
	
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		debug(ERROR,"system call","bind",0);
	
	if( listen(listenfd,64) <0)
		debug(ERROR,"system call","listen",0);
	
	while(1){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			debug(ERROR,"system call","accept",0);
		if((pid = fork()) < 0) {
			debug(ERROR,"system call","fork",0);
		}
		else {
			if(pid == 0) { 	// Proceso hijo
				(void)close(listenfd);
				process_web_request(socketfd); // El hijo termina tras llamar a esta función
			} else { 	// Proceso padre
				(void)close(socketfd);
			}
		}
	}
}
