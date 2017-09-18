/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdint.h>


#define BUFSIZE 1024
#define BUFLEN 1024


typedef struct packet {
  short op;
  union {
    unsigned short block;
    short code;
  };
  char *d;
  char *m;
} tftp_packet;


int error = 0;

void except(char *error) {
  perror(error);
  exit(1);
}

char *format_packet(tftp_packet packet, size_t *len) {
  *len = 0;
  *len += sizeof(short);
  char *raw = 0;
  short opcodeN = htons(packet.op);
  short errorcodeN = htons(packet.code);
  switch (packet.op) {
  case 01:
  case 02:
    *len += strlen(packet.m) + 1;
    raw = calloc(*len, 1);
    mempcpy(mempcpy(raw, &opcodeN, sizeof(short)), packet.m, strlen(packet.m) + 1);
    /* curPos = mempcpy(curPos, packet.m, strlen(packet.m)+1); */
    break;
  case 04:
    *len += sizeof(short);
    raw = calloc(*len, 1);
    mempcpy(mempcpy(raw, &opcodeN, sizeof(short)), &errorcodeN, sizeof(short));
    break;
  case 03:
    *len += sizeof(short);
    *len += strlen(packet.d);
    raw = calloc(*len, 1);
    mempcpy(mempcpy(mempcpy(raw, &opcodeN, sizeof(short)), &errorcodeN,
                    sizeof(short)),
            packet.d, strlen(packet.d));
    break;
  case 05:
    *len += sizeof(short);
    *len += strlen(packet.d) + 1;
    raw = calloc(*len, 1);
    mempcpy(mempcpy(mempcpy(raw, &opcodeN, sizeof(short)), &errorcodeN,
                    sizeof(short)),
            packet.d, strlen(packet.d) + 1);
    break;
  default:
    *len = 0;
  }
  return raw;
}

tftp_packet err_packet(int code) {
  tftp_packet *packet = calloc(1, sizeof(tftp_packet));
  packet->op = 05; // op of ERROR
  packet->code = code;
  packet->d = calloc(1, 20);
  
  if (code == 1) // code for File Not found
  {
    strcpy(packet->d, "File not found.");
  }
  
  return *packet;
}

tftp_packet * d_packet(char *buf, int numBytes, int blockNo) {
  tftp_packet *packet = calloc(1, sizeof(tftp_packet));
  packet->op = 03;
  packet->d = calloc(1, numBytes);
  memcpy(packet->d, buf, numBytes);
  packet->block = blockNo;
  return packet;
}

void sendPacket(tftp_packet packet, int sockfd, struct sockaddr_in sockClient) {

  size_t packetSize;
  char *buff = format_packet(packet, &packetSize);
  printf("Hello %s\n" ,buff);
  sendto(sockfd, buff, packetSize, 0, (struct sockaddr *)&sockClient, (socklen_t)sizeof(sockClient));
}





tftp_packet deformat_packet(char *buf, size_t packetLength) {
  tftp_packet *packetptr = calloc(1, sizeof(tftp_packet));
  memcpy(&packetptr->op, buf, sizeof(short));
  packetptr->op = ntohs(packetptr->op);

  switch (packetptr->op) {
  case 01:
  case 02:
    packetptr->m = strdup(buf + 2);
    packetptr->d = strdup(buf + strlen(packetptr->m) + 3);
    break;
  case 03:
    memcpy(&packetptr->block, buf + 2, sizeof(short));
    packetptr->block = ntohs(packetptr->block);
    packetptr->d = strndup(buf + 4, packetLength);
    break;
  case 04:
    memcpy(&packetptr->block, buf + 2, sizeof(short));
    packetptr->block = ntohs(packetptr->block);
    break;
  case 05:
    memcpy(&packetptr->block, buf + 2, sizeof(short));
    packetptr->block = ntohs(packetptr->block);
    packetptr->d = strndup(buf + 3, packetLength);
    break;
  default:
    packetptr->d = "\0";
    packetptr->m = "\0";
    ;
  }
  return *packetptr;
}


size_t recv_packet(tftp_packet *packet, int sockfd, struct sockaddr_in *sockClient) {
  size_t packetSize;
  char *buf = calloc(1, BUFLEN);

  packetSize = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)&sockClient,
                        (socklen_t *)sizeof(sockClient));
  if (packetSize > 0)
    *packet = deformat_packet(buf, packetSize);
  return packetSize;
}

int ack_packet(int sockfd, struct sockaddr_in sockClient, int blockNo) {
  tftp_packet packet;
  size_t packetLength = recv_packet(&packet, sockfd, &sockClient);
  if (packetLength > 0)
    if (packet.op == 04)
      if (packet.block == blockNo)
        return 1;
  return 0;
}



/*
 * error - wrapper for perror
 */
void err(char *msg) {
  perror(msg);
  exit(1);
}

void rrq(tftp_packet p, struct sockaddr_in c) {


  char *filename = strdup(p.m);



  struct sockaddr_in t_sock, c_sock;
  int sockfd = 0;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    except("ERROR OPENING SOCKET");
  bzero(&t_sock, sizeof(t_sock));
  bzero(&c_sock, sizeof(c_sock));


  c_sock.sin_port = c.sin_port;
  c_sock.sin_family = c.sin_family;
  c_sock.sin_addr = c.sin_addr;

  t_sock.sin_family = AF_INET;
  t_sock.sin_addr.s_addr = htonl(INADDR_ANY);
  t_sock.sin_port = htons(0); // Port 0 to get a random port
  int optval;
  optval = 1;

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
  if ((bind(sockfd, (struct sockaddr *)&t_sock, sizeof(t_sock)) < 0))
    except("FAILED TO BIND, IS THE ADDRESS ALREADY IN USE?");
  struct timeval read_timeout;
  read_timeout.tv_sec = 2;
  read_timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  if (access(filename, F_OK) == -1) {
    tftp_packet epacket = err_packet(1);
    perror(NULL);
    error = 1;
    sendPacket(epacket, sockfd, c_sock);
    return;
  }
  /* return; */

  /* Get random port to start transmission */

  int fd = open(filename, O_RDONLY);
  char *buf = calloc(1, 512);
  /* int packetLength = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr
  * *)&c_sock, */
  /*(socklen_t *)sizeof(c_sock)); */
  int numBytes, block = 1, retries = 3;
  while ((numBytes = read(fd, buf, 512)) > 0) {


    tftp_packet *packet = d_packet(buf, numBytes, block);
    int tries;
    for (tries = 0; tries < retries; tries++) {
      sendPacket(*packet, sockfd, c_sock);

      if (ack_packet(sockfd, c_sock, block))
        break;
    }
    if (tries == retries){
      break;
    }
    block++;
  }
  free(buf);
}


int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    err("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr)) < 0) 
    err("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr,(socklen_t *) &clientlen);
    if (n < 0)
      err("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			 //  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    // if (hostp == NULL)
    //   err("ERROR on gethostbyaddr");
    // hostaddrp = inet_ntoa(clientaddr.sin_addr);
    // if (hostaddrp == NULL)
    //   err("ERROR on inet_ntoa\n");
    // printf("server received datagram from %s (%s)\n", 
	   // hostp->h_name, hostaddrp);
    // printf("server received %ld/%d bytes: \n", strlen(buf), n);
    
    tftp_packet packet;

    for (int i = 0; i < n; ++i)
    {
      printf("%02X ",buf[i]);
    }

    printf("\n Packet Received \n");

   packet =  deformat_packet(buf, n);

    printf("\n Opcode : %d\n",packet.op);

    if (packet.op == 1)
    {
      
      printf("Read Request Received\n");

      rrq(packet, clientaddr);

    }
  
    }


}

