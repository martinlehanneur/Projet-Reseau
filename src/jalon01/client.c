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
#include <poll.h>
#include <pthread.h>



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


void* start_read(void *arg){
  while(1){
  char buf[1000];
  char message[100];
  do_read(*(int *)arg,buf,1000);
  if(strcmp(buf, "              [Server] Vous allez être déconnecté...\n")==0){
    printf("%s", buf);
    close (*(int *)arg);
    printf("Connection terminée\n");
    break;
  }
  else{
    printf ("%s\n",buf);
  }
}
exit(12);
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
    int sockfd = do_socket(AF_INET,SOCK_STREAM,0);



    //get the socket


    int len=100;
    char con_message[100];
    do_connect(sockfd,sin);
    do_read(sockfd,con_message,len);
    if(strcmp(con_message,"Le serveur n'accepte plus de connexions\n")==0){
      printf("%s\n", con_message);
      close (sockfd);
      exit(0);
    }
    printf("Entrer un message: ");

    pthread_t pthread[1];
    pthread_create(&pthread[1], NULL, start_read, (void*)&sockfd);

    //connect to remote socket
    while(1){
        char message[100];
        fgets(message, len, stdin);
        do_write(sockfd, message, len);
    }
  }
  exit(0);


}
