# Simple HTTP Server
This repository contains  a simple HTTP server made in C, capable of receiving HTTP requests and sending an appropiate response to the sender. It was developed as a semi guided project for a 3rd year class from the Computer Science degree at University of Murcia.

As a heads up, its a VERY rough server, where it handles the messages in a very basic, simple manner, with barely no concerns whatsoever about performance or security, so keep that in mind. It also has some features that were requested to be part of the project, which will be detailed later.

## Table of contents
- How to run the server
- Functionality
- Different sections of the code
- Debugging

## How to run the server

The server has been developed to run on a Linux Server, and all the testing and following commands have been done in said enviroment. As of the date of writing this readme file, the server is using certain libraries that are not supported by Windows by default, which might bring some issues when compiling or even when doing some of its functionality. It is highly advised to run it on a Linux Server.

To begin with, download the source code from the repository. You can either clone the repository if you want, or simply download the source code as a ZIP folder and then extract it.

Once thats done, the server has been programmed in C, and it requires to be compiled. To do so, use the gcc compiler with the following command:
```
gcc simple_http_server.c -o simple_server
```

This will compile the code into a binary file by the name of simple_server. To run the server, locate the folder where the binary and the rest of the files are and use the following command:
```
./simple_server [PORT] [ROOT]
```
As you can see, this command takes 2 arguments, which are a must for the server to be able to run properly. Those arguments are the following:

* **Port**: Indicates the port where the server will be listening to HTTP requests. The valid ports are between 1 to 60000.
* **Root**: Indicates the folder that contains all the files needed for the server to run, such as HTML pages. Its important to have permissions to read the folder contents.

## Functionality

The server is capable of the following things
* Receive a GET HTTP request and process it: the server is able to process a GET request, check its contents and formulate a response according to the data of the GET request, containing the necessary headers and the requested page.
* Receive a POST HTTP request and process it: the server is able to process a POST request which contains an email, its capable of extracting the email information, compare it to the expected email information, and formulate a response according to the results.
* Usage of cookies to restrict the amount of times a user can do a request to the server: the server uses a very basic cookie to handle the amount of times the user has done requests to the server, and time them out once they reach 10 requests. The cookie contains a max age of 60, which means that it expires after a minute.
* Basic error handling, such as forbidden paths or wrong formatted links: It checks the following things
    * Checks that the headers of the HTTP requests are formatted properly, and wont accept requests with an incorrect format
    * Checks that its not trying to access parent directories which should be out of the scope of the server
    * Checks that the requested file on a GET request has an extension, and if it has, checks if its a valid extension.
    * Checks the cookie value. If superior than the expected value, returns a 403
    * Checks that the requested file exists within the server contents, otherwise returns a 404

The valid extensions that server can handle on a GET request are the following:
| Extension | MIME       |
|-----------|------------|
| gif       | image/gif  |
| jpg       | image/jpg  |
| jpeg      | image/jpeg |
| png       | image/png  |
| ico       | image/ico  |
| zip       | image/zip  |
| gz        | image/gz   |
| tar       | image/tar  |
| htm       | text/htm   |
| html      | text/html  |


## Different sections of the code

The code has been structured in different sections, which are easy to distinguish one from another. Said sections are the following:
* Constants and extension struct
* Auxilary functions such as debugging, persistence and parse functions
* Response handler
* Request handler
    * Request parsing and error handling
    * GET request handler
    * POST request handler
* Main function with socket handling

While its true that the code is divided in those sections, it is also correct to say that the code inside of these sections can at time get messy and might be hard to understand. A lot of it was rushed and with the aim of making "something that works", but it should still be readable enough. It could be heavily improved, in particular it could've been splitted in more, easier to read functions, but for now this project is deeemed as completed with no further improvements in the near future.

## Debugging

The debugging on the server is done by logging debug messages on a log file named webserver.log. In there, its notified in most situations what went wrong, with some information attached to it. Its possible to add debug codes by modifying the debug function included in the server's code.

The debug function is the following and takes 4 arguments in total:
```
void debug(int log_message_type, char *message, char *additional_info, int socket_fd)
```

The parameters are very self explanatory, but in any case, they are explained ahead:
* **log_message_type**: Indicates the code of the debug message. Use one of the defined constants of the server, or add a new one for a different code. This will require further modifications of the function for its handling.
* **message**: Basic information about the debug message. Short and concise.
* **additional_info**: Additional info that might be needed for the proper understanding of what went wrong.
* **socket_fd**: File descriptor of the socket on which the server is listening to requests

### Final notes
_**(ESP)**_
Si eres de la fium y has encontrado esto, solo decir que no merece la pena. No te digo que estudies este repositorio, pero no merece la pena para nada utilizarlo como base. Cada año se cambia la práctica, y el código de este repositorio no es bueno, limpio o eficiente. Simplemente funciona a medias, con parcheos de por medio para que todo funcione como debería. Hay maneras mejores de realizar esta práctica, y es una práctica que puede ser complicada en ciertos aspectos, pero que con un poco de trabajo se saca adelante. Siempre piensa que lo que estas haciendo, al fin y al cabo, es simplemente parsear una cadena de caracteres, y crear otra nueva como respuesta.

Por último decir que si un profesor tiene algun problema con el respositorio, mandar un mensaje a gonzalo.nicolasm@um.es y pondré el repositorio en privado al momento.

Este repositorio, como experience personal, fue algo exhaustivo, estresante, y algo que a al larga sinceramente no me dio una gran satisfacción, puesto que en la actualidad ya existen servidores HTTP de gran calidad y libres tales como Apache. Bien es cierto que esos servidores no son perfectos, siempre se pueden mejorar, y que esto es una simple práctica, un "mockup", y que por tanto no debería de tomarse muy enserio. Pero esta práctica me llego a limite, en ocasiones llevandome a la desesperación, puesto que el uso de la funcion select() es MUY sensible, y la generacion de respuestas por algun motivo no le gustaba como lo realizaba, pese a que en un principio, ambas formas que utilicé realizaban lo mismo. En fin, el caso es que esta completado y que la asignatura esta aprobada.

_**(EN)**_
Honestly I don't know what to put in here. This is pretty much just a mockup http server done for a 3rd year class in CS. It really shouldn't be taken too seriously, its a simple display of what I was capable to do in a month or 2 with C and HTTP requests knowledge. If I feel like, I might add some more content in the foot notes, but I really don't think that will happen.
