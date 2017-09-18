
#include <dirent.h> 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int parentfd; 
  int childfd; 
  int portno; 
  int clientlen; 
  struct sockaddr_in serveraddr; 
  struct sockaddr_in clientaddr; 
  struct hostent *hostp; 
  char buf[BUFSIZE]; 
  char *hostaddrp; 
  int optval; 
  int n; 

  
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  
  bzero((char *) &serveraddr, sizeof(serveraddr));

  
  serveraddr.sin_family = AF_INET;

  
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  
  serveraddr.sin_port = htons((unsigned short)portno);

  
  if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

  
  clientlen = sizeof(clientaddr);
  while (1) {
    
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
    if (childfd < 0) 
      error("ERROR on accept");
    
    
    bzero(buf, BUFSIZE);
    n = read(childfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    
    printf("server received %d bytes: %s", n, buf);
    


    char response[1024];
    bzero(response, BUFSIZE);

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
      
      DIR *d;
      strcpy(response,"File List\n");
      struct dirent *dir;
      d = opendir(".");

      if (d)
      {
        while ((dir = readdir(d)) != NULL)
        {
          strcat(response,dir->d_name);
          strcat(response,"\n");
        }
        strcat(response,"\0");
        closedir(d);
      }

      n = write(childfd, response, strlen(response));
      if (n < 0) 
      error("ERROR writing to socket");

    }
    else if (strcmp(get,"get\0") == 0)
    {
        int length = strlen(buf);
        int file_length = length - 4;
        char filename[file_length];

        strcpy(filename,&buf[4]);

        filename[strlen(filename)-1] = '\0';

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

        n = write(childfd, string, strlen(string));
        if (n < 0) 
         error("ERROR writing to socket");
        }
        else{
          error("File doesnot exist");
        }

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

        FILE *f = fopen(filename,"w+");

        n = read(childfd, response, BUFSIZE);

        fprintf(f, "%s", response);
        fclose(f);

    }
    else{
      strcpy(response,"\nCommand Unrecognized\0");

      n = write(childfd, response, strlen(response));
      if (n < 0) 
      error("ERROR writing to socket");

    }



    
    
    close(childfd);
  }
}