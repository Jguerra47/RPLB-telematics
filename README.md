
# Proxy reverso + balanceador de cargas

En este proyecto se implementa una proxy inversa con un balanceador de cargas desarrollado en el lenguaje de programación c, 
en el cual se utiliza una arquitectura cliente servidor usando api sockets. Durante el proyecto se hizo el análisis de las 
diferentes estrategias, algunas serán mencionadas en la sesión de desarrollo, para hacer una implementación correcta y poder 
ampliar el conocimiento en el área.

## Desarrollo


El proyecto desarrollado cuenta con la siguiente arquitectura en la cual se cuenta con tres servidores desplegados
en una instancia ec2 de aws que tienen almacenada una aplicación desarrollada en golang, además, hay un reverse proxy
que se encarga de redireccionar las peticiones del cliente entre los diferentes servidores de acuerdo al orden de llegada
usando una política de balanceo de cargas round robin y por ultimo esta el cliente que es el encargado de ejecutar las peticiones.

![Arquitectura](https://i.postimg.cc/q7dFRvpB/Diagrama-sin-t-tulo-drawio.png)
## API Desplegada en los servers

API con funcionamientos basicos desarrollada con el objetivo de comprobar el correcto funcionamiento de la proxy reversa y el balanceador de
cargas, esta API esta replicada en tres servidores para evidenciar el escalado horizontal y el funcionmiento de la politica round robin utilizada
para la proxy reversa.

Caraxteristicas de la apliacion:
- Desarrollada en go
- Desplegada en los servidores ec2 de AWS
- Puerto de escucha: 8080


#### Get path por defecto

```http
  GET /
```
ejemplo de la response
```http
  {
    "message":"Default"
  }
```
#### Get para test

```http
  GET /ping
```
Ejemplo de la response
```http
  {
    "message":"pong",
    "status":"200"
  }
```

#### ¿Como probar el funcionamiento de la aplicacion?

ip = IP asiganda a la maquina

request:
```http
GET / HTTP/1.1
Host: IP:8080
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/106.0.5249.62 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,/;q=0.8,application/signed-exchange;v=b3;q=0.9
Accept-Encoding: gzip, deflate
Accept-Language: es-419,es;q=0.9
Connection: close
```

Para correr el codigo del servidor se debe ejecutar los siguientes comandos en el directorio del servidor:
```
go mod tidy
go run servidor
```




## Proxy

El proxy reverso esta implementado en C, se escogio este lenguaje con el objetivo de afrontar un nuevo reto
durante el desarrollo. El proxy reverso funciona de la siguiente manera:
- Recibe una peticion del cliente mediante un socket
- Ejectua mediante hilos una funcion que valida la peticion, si la respuesta esta en los archivos de cache y el TTL del  archivo es valido se retorna el contenido del archivo, caso contrario se crea un socket con uno de los tres servidores, el cual es elegido bajo la metodologia Round Robin, y envia la peticion al servidor 
- Luego se captuora la respuesta del servidor
- Se actualiza la cache
- Por ultimo, se envia al cliente la respuesta

TTL de 30 segundos

#### ¿Como ejecutar el proxy?
para ejecutar el proxy debemos correr los siguientes comandos en la ubicacion del archivo rproxy.c


```
  gcc -o executable rproxy -pthread
  ./executable
```

o directamente se ejecuta el archivo executable mediante
```
  ./executable
```

#### ¿Como probar funcionalidad?
IP = IP publica de la proxy

Puerto:8080

una vez esten corriendo los servidores y la proxy se puede lanzar request desde un navegador de la siguiente manera
```
  IP:8080/ 
  IP:8080/ping
```
## ¿Son los hilos la mejor opción para soportar la concurrencia?

Los hilos son una buena solucion cuando la proxy va a soportar pocas peticiones debido a que con muchas peticiones la 
escalabilidad se puede complicar, cuando se van a recibir muchas peticiones, la mejor solucion son los  epoll en c, 
para mas informacion https://www.gilesthomas.com/2013/09/writing-a-reverse-proxyloadbalancer-from-the-ground-up-in-c-part-2-handling-multiple-connections-with-epoll

## Paginas y videos de ayuda
- https://www.geeksforgeeks.org/socket-programming-cc/
- https://www.gilesthomas.com/2013/08/writing-a-reverse-proxyloadbalancer-from-the-ground-up-in-c-part-0
- https://www.youtube.com/watch?v=sCR3SAVdyCc&t=1s&ab_channel=IBMTechnology

## Authors

- [@DanielHernandezO](https://www.github.com/DanielHernandezO)
- [@jguerra47](https://www.github.com/jguerra47)
- [@jacevar](https://www.github.com/jacevar)
