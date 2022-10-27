// Dependencies
#define _OPEN_SYS_ITOA_EXT
#define CONFIG_ARG_MAX_BYTES 128

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <locale.h>

typedef struct config_option config_option;
typedef config_option* config_option_t;

struct config_option {
    config_option_t prev;
    char key[CONFIG_ARG_MAX_BYTES];
    char value[CONFIG_ARG_MAX_BYTES];
};

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
    if((seconds-lastTime)<10)return 1;
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

config_option_t read_config_file(char* path) {
    FILE* fp;
    
    if ((fp = fopen(path, "r+")) == NULL) {
        perror("fopen()");
        return NULL;
    }

    config_option_t last_co_addr = NULL;
    while(1) {
        config_option_t co = NULL;
        if ((co = calloc(1, sizeof(config_option))) == NULL)
            continue;
        memset(co, 0, sizeof(config_option));
        co->prev = last_co_addr;

        if (fscanf(fp, "%s = %s", &co->key[0], &co->value[0]) != 2) {
            if (feof(fp)) {
                break;
            }
            if (co->key[0] == '#') {
                while (fgetc(fp) != '\n') {
                    // Do nothing (to move the cursor to the end of the line).
                }
                free(co);
                continue;
            }
            perror("fscanf()");
            free(co);
            continue;
        }
        //printf("Key: %s\nValue: %s\n", co->key, co->value);
        last_co_addr = co;
    }
    return last_co_addr;
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
                    break;
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
    config_option_t co;
    if ((co = read_config_file("config.conf")) == NULL) {
        perror("read_config_file()");
        return -1;
    }

    pthread_t tid;
    char port[100],ip[100];
    char proxy_port[100];
    // accept arguments from terminal
    strcpy(port,co->value);  // server port
    co = co->prev;
    strcpy(proxy_port,co->value); // proxy port
    //hostname_to_ip(hostname , ip);
    co = co->prev;
    //List of available servers adresses

    char *ips[3];
    ips[0] = co->value; co->prev; //"3.86.215.239";
    ips[1] = co->value; co->prev; //"54.205.146.5";
    ips[2] = co->value; co->prev; //"18.233.7.120";

    printf("server IP : %s and port %s" ,ips[2],port);
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
    short counter=0; //Defined for round-robin strategy loadbalancer
    while(1){
        client_fd = accept(proxy_fd, (struct sockaddr*)NULL ,NULL);
        if(client_fd > 0){
            // Multithreading variables
            struct serverInfo *item = malloc(sizeof(struct serverInfo));
            item->client_fd = client_fd;
            strcpy(item->ip,ips[counter]);
            strcpy(item->port,port);
            pthread_create(&tid, NULL, runSocket, (void *)item);

            //Change server for the next request
            counter++;
            counter = counter%3;
            sleep(1);
        }
    }
    
    return 0;
}
