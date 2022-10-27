// Dependencies
#define _OPEN_SYS_ITOA_EXT
#include <stdio.h>
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
#include <locale.h>

// A structure to maintain client fd, and server ip address and port address
// client will establish connection to server using given IP and port
struct serverInfo{
    int client_fd;
    char ip[100];
    char port[100];
};

// Used to check time during a request
void getTime(char* path){
    FILE *fileP;
    fileP = fopen(path, "w") ;
    time_t now;
    time(&now);
    long long seconds = now;
    char buffer[35];
    sprintf(buffer,"%lld",seconds);
    fputs(buffer,fileP);
    fputs("\n",fileP);
    fclose(fileP);
}

int checkTTL(long long lastTime){
    time_t now;
    time(&now);
    long long seconds = now;
    if((seconds-lastTime)<30)return 1;
    return 0;
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
    FILE *filePointer;
    FILE *fpCache;
    if(isFileExists("Logs.txt")==0){
        perror("error opening file Logs.txt");
        exit(0);
    }
    filePointer = fopen("Logs.txt", "a") ;
    struct serverInfo *info = (struct serverInfo *)vargp;
    char buffer[30000];
    int bytes =0;
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
    printf("server socket created...\n");
    memset(&server_sd, 0, sizeof(server_sd));
    // set socket variables
    server_sd.sin_family = AF_INET;
    server_sd.sin_port = htons(atoi(info->port));
    server_sd.sin_addr.s_addr = inet_addr(info->ip);
    //connect to main server from this proxy server
    if((connect(server_fd, (struct sockaddr *)&server_sd, sizeof(server_sd)))<0){
        printf("server connection not established");
    }
    printf("server socket connected...\n");
    //receive data from client
    memset(&buffer, '\0', sizeof(buffer));
    bytes = read(info->client_fd, buffer, sizeof(buffer));
    char path[25];
    if(bytes > 0){
        // send data to main server
        //printf("client fd is : %d\n",c_fd);
        printf("\n\nRequest :\n");
        short counter=0;
        fputs(buffer,filePointer);
        fputs(buffer,stdout);
        for(int i=0;i<sizeof(buffer);i++){
            if(counter==2){
                char ext[] = {'.','t','x','t'};
                strncat(path,ext,4);
                break;
            }
            if(buffer[i]==47)continue;
            if(buffer[i]==32){
                counter++;
                char aux= '-';
                strncat(path,&aux,1);
                continue;
            }
            strncat(path,&buffer[i],1);
        }
        int flag=0;
        char lastDate[20];
        char bufferCache[30000];
        memset(&lastDate, '\0', sizeof(lastDate));
        memset(&bufferCache, '\0', sizeof(bufferCache));
        if(isFileExists(path)==1){
            fpCache = fopen(path, "r");
            int c;
            int i=0,j=0;
            while(1) {
                c = fgetc(fpCache);
                if( feof(fpCache) ) { 
                    break ;
                }
                char ch = c;
                if(flag==1){
                    bufferCache[i] = ch;i++;
                }else if(ch =='\n'){
                    flag=1;
                }else{
                    lastDate[j]=ch;
                    j++;
                }
            }
            long long currentTime=0;
            currentTime = strtoll(lastDate, NULL, 10);
            if(checkTTL(currentTime)==1){
                fputs("*********Using cache*********\n",stdout);
                fputs("response",stdout);
                fputs("\n",stdout);
                fputs(bufferCache,stdout);
                fputs("\n",stdout);
                write(info->client_fd,bufferCache, sizeof(bufferCache));
                fclose(fpCache);
                fclose(filePointer);
                return NULL;
            }
           fputs("*********Invalid TTL********\n",stdout);
        }
        fpCache = fopen(path, "a") ;
        fputs(buffer,stdout);
        fflush(stdout);
        sleep(2);
    }
    write(server_fd, buffer, sizeof(buffer));
    //Recieve response from server
    memset(&buffer, '\0', sizeof(buffer));
    bytes = read(server_fd, buffer, sizeof(buffer));
    if(bytes > 0){
        // send response back to client
        write(info->client_fd, buffer, sizeof(buffer));
        fputs("Response :\n",stdout);
        fputs(buffer,stdout);
        fputs("\n",filePointer);
        fputs(buffer,filePointer);
        fputs("\n\n",filePointer);
        getTime(path);
        fputs(buffer,fpCache);
    }
    fclose(filePointer) ;
    fclose(fpCache) ;
    return NULL;
}

// main entry point
int main(int argc,char *argv[]){
    pthread_t tid;
    char port[100],ip[100];
    char proxy_port[100];
    // accept arguments from terminal
    strcpy(port,"8080");  // server port
    strcpy(proxy_port,"8080"); // proxy port
    //hostname_to_ip(hostname , ip);

    //List of available servers adresses
    char *ips[3];
    ips[0] = "3.86.215.239";
    ips[1] = "3.86.215.239";
    ips[2] = "3.86.215.239";
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
            sleep(1);
        }
    }
    
    return 0;
}
