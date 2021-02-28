#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define VERSION		24
#define BUFSIZE		8096
#define ERROR		42
#define LOG			44
#define PROHIBIDO	403
#define NOENCONTRADO	404
#define OK	200
#define BAD_REQUEST	400
#define NOT_ALLOWED	405
#define NOT_IMPLEMENTED 501
#define END_CHAR	'\0'	
#define GET_METHOD	1
#define POST_METHOD	2


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
		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",message, additional_info, socket_fd); break;
	}

	if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
	}
	if(log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO) exit(3);
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


void process_web_request(int descriptorFichero)
{
	debug(LOG,"request","Ha llegado una peticion",descriptorFichero);
	//
	// Definir buffer y variables necesarias para leer las peticiones
	//

	
	fd_set fdset;	// Conjunto de descriptores de ficheros.
	struct timeval tv;		// Timeout
	int retval;		// Valor devuelto por select()
	

	// Queremos trabajar con el fd  "descriptorFichero"
	// Vacio el conjunto de descriptores de fichero
	// Añado el descriptor de fichero al conjunto

	FD_ZERO(&fdset); 
	FD_SET(descriptorFichero, &fdset);

	// Espero hasta 5 segundos (Siguiendo el ejemplo de man 2 select)
	// En mi caso, eligo 30 segundos (temporal)

	tv.tv_sec = 30;
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

		//debug(LOG, "REQUEST\n====\n", request_line, descriptorFichero);

		char * metodo;	// Método (GET/POST)
		char * path;	// Path del objeto que se pide
		char * protocolo;	// Protocol HTTP
		char * save_ptr1;	// Usado para mantener la posicion en strtok_r()

		metodo = strtok_r(request_line, " ", &save_ptr1);
		path = strtok_r(NULL, " ", &save_ptr1);
		protocolo = strtok_r(NULL, " ", &save_ptr1);

		//debug(LOG, "\nMETODO", metodo, descriptorFichero);

		char * header_line;		// Primera linea de cabecera HTTP
		char * host;			// Cadena correspondiente a "Host:""
		char * server;			// Cadena correspondiente a la direccion del servidor
		char * save_ptr2;		// Usado para mantener la posicion en strtok_r() de la primera linea de cabecera
		
		header_line = strtok_r(NULL, "\r\n", &save_ptrl);	// Primera linea de cabecera
		//debug(LOG, "HEADER\n====\n", header_line, descriptorFichero);

		if (header_line != NULL) {		// Compruebo si hay cabecera 

			host = strtok_r(header_line, " ", &save_ptr2);
			server = strtok_r(NULL, " ", &save_ptr2 );

			//debug(LOG, "\nSERVER", server, descriptorFichero);

		} else {

			debug(ERROR, "Header error", "No existe cabecera HTTP", descriptorFichero);
			close(descriptorFichero);

		}

		
		//
		//	TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
		//	(Se soporta solo GET)
		//
		//debug(LOG, "TEST  MSG", "TEST", descriptorFichero);


		// Dependiendo de que código me devuelva la funcion, hago algo
		// Compruebo los casos de ERROR. Mas facil manejar

		int method_code = parse_method(metodo);
		if (method_code > 0){
			switch (method_code)
			{
			case -1:
				debug(ERROR, "Method error, code", BAD_REQUEST, descriptorFichero);
				break;
			
			default:
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
	if(fork() != 0)
		return 0; // El proceso padre devuelve un OK al shell

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
