#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

void error (const char *msg){
  perror(msg);
  exit(1);
}


int do_socket(int domaine, int type, int protocol){
  int yes=1;

  int sockfd= socket(domaine, type, protocol);
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  error("ERROR setting socket options");

  return sockfd;
}

void do_connect(int sockfd, struct sockaddr_in sin){
  if (connect(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0) {
    error("Client: connect");
    exit(1);
  }
}

void do_write(int sockfd, char *message, int len){
if(write(sockfd,message,len)<0){
  error("client: write");
  exit(1);
}
}

void do_read(int sockfd, char *buf, int len){
if(read(sockfd,buf,len)<0){
  error("client: read");
  exit(1);
}
}


int main(int argc,char** argv)
{


  if (argc != 3)
  {
    fprintf(stderr,"usage: RE216_CLIENT hostname port\n");
    return 1;
  }
  else {

    //get address info from the server
    struct sockaddr_in sin;
    sin.sin_family=AF_INET;
    sin.sin_port=htons(atoi(argv[2]));
    inet_aton(argv[1], &sin.sin_addr);

    //get the socket

    int sockfd = do_socket(AF_INET,SOCK_STREAM,0);
    do_connect(sockfd,sin);
    int len=100;
    char con_message[100];

    do_read(sockfd,con_message,len);
    if(strcmp(con_message,"Le serveur n'accepte plus de connexions\n")==0){
printf("[Server] : %s\n", con_message);
      close (sockfd);
      exit(0);
    }


    //connect to remote socket
    while(1){
    char buf[1000];
    char buf2[100];
    char message[100];

    printf("Entrer un message: ");
    fgets(message, len, stdin);
    do_write(sockfd,message, len);
    do_read(sockfd,buf,1000);
    if(strcmp(message, "/quit\n")==0){
      printf("[Server] : %s\n", buf);
      close (sockfd);
      printf("Connection terminée\n");
      exit(0);
    }
else{
    printf ("[Serveur] : %s\n",buf);
  }
  }

  }

  exit(0);


}
