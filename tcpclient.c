
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


#define BUFSIZE 1024


void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    
    if (connect(sockfd,(struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    
    printf("Please enter Command: ");
    bzero(buf, BUFSIZE);
    fgets(buf, BUFSIZE, stdin);

    
    n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");

    char response[1024];
    bzero(response, BUFSIZE);

    n = read(sockfd, response, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");

    char get[4];
    if (strlen(buf) > 4)
    {
        strncpy(get,buf,3);
        get[3] = '\0';

    }

    char put[4];
    if (strlen(buf) > 4)
    {
        strncpy(put,buf,3);
        put[3] = '\0';

    }

    if (strcmp(buf,"ls\n") == 0)
    {
        printf("Return from server:\n %s", response);    
    }
    else if (strcmp(get,"get\0") == 0)
    {
        int length = strlen(buf);
        int file_length = length - 4;
        char filename[file_length+1];

        strncpy(filename,&buf[4],file_length-1);

        filename[strlen(filename)-1] = '\0';

        printf("%s\n",filename );
        

        printf("Return from server:\n %s", response);

        FILE* f = fopen(filename,"w+");
        fprintf(f, "%s", response);
        fclose(f);

    }
    else if (strcmp(put,"put\0") == 0)
    {
        int length = strlen(buf);
        int file_length = length - 4;
        char filename[file_length];

        strcpy(filename,&buf[4]);
        filename[strlen(filename)-1] = '\0';
        strcat(filename,"\0");

        printf("%s\n",filename );
        
        // printf("Return from server:\n %s", response);

        FILE *f = fopen(filename,"r");

        if (f != NULL)
        {
          /* code */
        
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);  //same as rewind(f);

        char *string = malloc(fsize + 1);
        fread(string, fsize, 1, f);
        fclose(f);

        string[fsize] = 0;


        n = write(sockfd, string, strlen(string));
        if (n < 0) 
         error("ERROR writing to socket");
        }
        else{
          error("File doesnot exist");
        }

    }
    

    close(sockfd);
    return 0;
}

