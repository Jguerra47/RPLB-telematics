// Dependencies
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <time.h>

// A structure to maintain client fd, and server ip address and port address
// client will establish connection to server using given IP and port
struct serverInfo{
    int client_fd;
    char ip[100];
    char port[100];
};

//A structure who lets us match data from cache and store time creation for TTL purpouse
struct pair{
    char *name;
    time_t time;
};

// Used to check time during a request
time_t getTime(){
    time_t now;
    time(&now);
    return now;
}

// File checker
int isFileExists(const char *path){
    FILE *fptr = fopen(path, "r");
    if(fptr==NULL){
        return 0;
    }
    fclose(fptr);
    return 1;
}

void *runSocket(void *vargp){
    struct serverInfo *info = (struct serverInfo *)vargp;
    char buffer[30000];
    int bytes =0;
    printf("client:%d\n",info->client_fd);
    fputs(info->ip,stdout);
    fputs(info->port,stdout);
    //code to connect to main server via this proxy server
    int server_fd =0;
    struct sockaddr_in server_sd;
    // create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        printf("server socket not created\n");
    }
    printf("server socket created\n");
    memset(&server_sd, 0, sizeof(server_sd));
    // set socket variables
    server_sd.sin_family = AF_INET;
    server_sd.sin_port = htons(atoi(info->port));
    server_sd.sin_addr.s_addr = inet_addr(info->ip);
    //connect to main server from this proxy server
    if((connect(server_fd, (struct sockaddr *)&server_sd, sizeof(server_sd)))<0){
        printf("server connection not established");
    }
    printf("server socket connected\n");
    //receive data from client
    memset(&buffer, '\0', sizeof(buffer));
    bytes = read(info->client_fd, buffer, sizeof(buffer));
    if(bytes > 0){
        // send data to main server
        write(server_fd, buffer, sizeof(buffer));
        //printf("client fd is : %d\n",c_fd);
        printf("From client :\n");
        char path[25];
        short counter=0;
        for(int i=0;i<sizeof(buffer);i++){
            if(counter==2)break;
            if(buffer[i]==47)continue;
            if(buffer[i]==32)counter++;
            strncat(path,&buffer[i],1);
        }
        printf("\n\npath: %s \n",path);
        struct pair *value = malloc(sizeof(struct pair));
        value->name= path;
        value->time=getTime();
        fputs(buffer,stdout);
        fflush(stdout);
        sleep(2);

        //Check if file exists to handle body response
        if(isFileExists(path)==0){
            printf("*****************The file does not exists*************\n");
        }
        else{
            //As it exists, check if it has expired according to TTL parameter
        }

        double seconds = difftime(getTime(),value->time);
        printf("\n\nseconds: %.f\n\n",seconds);
    }

    //Recieve response from server
    memset(&buffer, '\0', sizeof(buffer));
    bytes = read(server_fd, buffer, sizeof(buffer));
    if(bytes > 0){
        // send response back to client
        write(info->client_fd, buffer, sizeof(buffer));
        printf("From server :\n");
        printf("\n\n%s\n\n",buffer);
        //fputs(buffer,stdout);
    }
    return NULL;
}

// main entry point
int main(int argc,char *argv[]){
    pthread_t tid;
    char port[100],ip[100];
    char *hostname = argv[1];
    char proxy_port[100];
    // accept arguments from terminal
    strcpy(ip,argv[1]); // server ip
    strcpy(port,argv[2]);  // server port
    strcpy(proxy_port,argv[3]); // proxy port
    //hostname_to_ip(hostname , ip);

    //List of available servers adresses
    char *ips[3];
    //ips[0] = ${SRV_ADDRESS_1};
    //ips[1] = ${SRV_ADDRESS_2};
    //ips[2] = ${SRV_ADDRESS_3};
    printf("server IP : %s and port %s" , ip,port);
    printf("proxy port is %s",proxy_port);
    printf("\n");

    //Socket variables
    int proxy_fd =0, client_fd=0;
    struct sockaddr_in proxy_sd;

    // add this line only if server exits when client exits
    signal(SIGPIPE,SIG_IGN);

    // Create a socket
    if((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nFailed to create socket");
    }
    printf("Proxy created\n");

    //Release memory for sockaddr_in
    memset(&proxy_sd, 0, sizeof(proxy_sd));

    // Set socket variables
    proxy_sd.sin_family = AF_INET;
    proxy_sd.sin_port = htons(atoi(proxy_port));
    proxy_sd.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if((bind(proxy_fd, (struct sockaddr*)&proxy_sd,sizeof(proxy_sd))) < 0){
        printf("Failed to bind a socket");
    }
    // Start listening to the port for new connections
    if((listen(proxy_fd, SOMAXCONN)) < 0){
        printf("Failed to listen");
    }
    printf("Waiting for connection..\n");

    // Accept all client connections continuously
    short counter=1; //Defined for round-robin strategy loadbalancer
    while(1){
        client_fd = accept(proxy_fd, (struct sockaddr*)NULL ,NULL);
        printf("Client no. %d connected\n", client_fd);
        if(client_fd > 0){
            // Multithreading variables
            struct serverInfo *item = malloc(sizeof(struct serverInfo));
            item->client_fd = client_fd;
            strcpy(item->ip,ips[counter%3]);
            strcpy(item->port,port);
            pthread_create(&tid, NULL, runSocket, (void *)item);

            //Change server for the next request
            counter = counter%3;
            counter++;
            printf("\n\n Send to server instance: %d\n\n",counter);
            sleep(1);
        }
    }
    return 0;
}
