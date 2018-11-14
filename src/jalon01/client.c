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
#include <signal.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<assert.h>

//PORT used for peer-to-peer
int PORT=8890;

//Mutex initialization
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t verrou2 = PTHREAD_MUTEX_INITIALIZER;


void error (const char *msg){
  perror(msg);
  exit(1);
}

void do_bind(int sockfd,struct sockaddr_in6 sin){
  if (bind(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0 ){
    error("Server : bind");
    exit (1);
  }
}

void do_listen(int sockfd, int nb_connexions){
  if (listen(sockfd,nb_connexions)<0){
    error("Server : listn");
    exit (1);
  }
}

//Accept connection
int do_accept(int sockfd,struct sockaddr_in6 sinclient){
  socklen_t len;
  len = sizeof sinclient;
  int var = accept(sockfd,(struct sockaddr*)&sinclient,&len);
  if(var<0){
    error("server: accept");
  }
  return var;
}

//Socket Creation
int do_socket(int domaine, int type, int protocol){
  int yes=1;
  int sockfd= socket(domaine, type, protocol);
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  error("ERROR setting socket options");

  return sockfd;
}

//Connection to server with ipv6 adress
void do_connect6(int sockfd, struct sockaddr_in6 sin){
  if (connect(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0) {
    error("Client: connect");
    exit(1);
  }
}

//Connection to server with ipv4 adress
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

//SIGINT signal redirection
void traitant(int a){
  printf("\n              /quit if you want to quit\n ");

}

//Thread for receive a file
void* start_read_send(void* arg){
  char message[100]="";
  char message2[100]="";
  char message3[100]="";
  char message4[100]="";
  char message5[100]="";
  char message6[100]="";
  char* buf=(char *)arg;
  char* adresse=strtok(buf+6, " ");
  char* nom_envoyeur=strtok(NULL, " ");
  char* nom_fichier=strtok(NULL, " ");
  char chemin_fichier[100]="";
  int len=100;

  //Find if server adress is an ipv6 or ipv4 adress
  struct addrinfo *res, hints;
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;
  getaddrinfo(adresse, NULL, &hints, &res);
  int sockfd;
  sleep(3);
  //The server is configure as ipv6
  if(res->ai_family==AF_INET6){
    struct sockaddr_in6 sin;
    sin.sin6_port=htons(PORT);
    sin.sin6_family=AF_INET6;
    inet_pton(AF_INET6, adresse, &sin.sin6_addr);
    sockfd = do_socket(res->ai_family,SOCK_STREAM,0);
    do_connect6(sockfd,sin);
  }
  //The server is configure as ipv4
  else if(res->ai_family==AF_INET){
    struct sockaddr_in sin;
    sin.sin_port=htons(PORT);
    sin.sin_family=AF_INET;
    inet_aton(adresse, &sin.sin_addr);
    sockfd = do_socket(res->ai_family,SOCK_STREAM,0);
    do_connect(sockfd,sin);
  }


  //Wait the server


  printf("Please tap enter to see the message \n");
  strcat(message, nom_envoyeur);
  strcat(message, " vous envoie le fichier ");
  strcat(message, nom_fichier);
  strcat(message, "\nAcceptez vous le fichier [y/n]\n");
  strcat(message2, "Fichier accepté\n");
  strcat(message3, " Fichier transféré\n");
  strcat(message4, nom_fichier);
  strcat(message4, " sauvegardé dans /Projet-Reseau/");
  strcat(message4, nom_fichier);
  strcat(message5, "Fichier refusé\n");
  strcat(chemin_fichier, "../../");
  strcat(chemin_fichier, nom_fichier);

  //Take a mutex for fgets
  pthread_mutex_lock(&verrou);
  printf("%s", message);
  while(1){
    char reponse[len];

    fgets(reponse, len, stdin);
    if(strncmp(reponse, "y",1)==0 ){ //The response is yes for the file transfert
      FILE* fichier;
      fichier=fopen(chemin_fichier,"w");
      __sync_synchronize();
      do_write(sockfd, message2, len); //Write to the server that we accept the file
      __sync_synchronize();
      int size;
      read(sockfd, &size, sizeof(int));
      char buf2[size];
      memset(buf2, 0, size);
      __sync_synchronize();
      do_read(sockfd,buf2,size); //Read what the server sends
      __sync_synchronize();
      fwrite(buf2, 1, sizeof(buf2), fichier);
      printf("              %s\n",message3);
      printf("              %s\n",message4);
      fclose(fichier);
      close(sockfd);
    }
    else{
      __sync_synchronize();
      do_write(sockfd, message5, len);//Write to the server that we don't accept the file
      __sync_synchronize();
      break;
    }
    break;

  }
  pthread_mutex_unlock(&verrou);//Free the mutex
}

//Thread for sending a file
void* send_folder(void *arg){
  int len=100;
  char buf[1000];
  char message3[100]="";
  char message[100]="";
  int taille_max=70;


  //Server initialization for both ipv4 and ipv6
  struct sockaddr_in6 sin;
  struct sockaddr_in6 sinclient;
  socklen_t* addrlen= (socklen_t*)sizeof(sinclient);
  sin.sin6_family=AF_INET6;
  sin.sin6_port=htons(PORT);
  sin.sin6_addr=in6addr_any;

  //Socket creation
  int sockfd = do_socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
  do_bind(sockfd, sin);
  do_listen(sockfd, 2);
  int connexion=do_accept(sockfd, sinclient);
  while(1){
    do_read(connexion, message, len); //Read the response of the client for the file transfert
    if(strcmp(message,"Fichier refusé\n" )==0){
      printf("              %s\n", message);
    }
    else{
      printf("              %s\n", message);
      char* nom_fichier=(char *)arg;
      FILE* fichier;;
      fichier=fopen(nom_fichier, "r");
      if(fichier!=NULL){
        strcat(message3, "Fichier tranféré\n");
        int size;
        fseek(fichier, 0, SEEK_END);
        size = ftell(fichier);
        fseek(fichier, 0, SEEK_SET);
        write(connexion, &size, sizeof(size));
        char texte[size];
        while(!feof(fichier)){//Read the file to send
          fread(texte, 1, sizeof(texte), fichier);
          __sync_synchronize();
          do_write(connexion, texte, sizeof(texte));//Send the file
          __sync_synchronize();
          bzero(texte, sizeof(texte));
          sleep(0.1);
        }
        __sync_synchronize();
        printf("              %s\n", message3);
        close(connexion);
        close(sockfd);
        fclose(fichier);
        __sync_synchronize();
      }
      else{
        printf("                  Ce fichier n'existe pas \n");
      }
      break;
    }
    break;
  }
}

//Thread read all what is send on the socket
void* start_read(void *arg){
  while(1){
    char buf[1000];
    char message[100]="";
    pthread_mutex_lock(&verrou2);
    __sync_synchronize();
    do_read(*(int *)arg,buf,1000);
    __sync_synchronize();
    pthread_mutex_unlock(&verrou2);
    sleep(0.1);

    if(strcmp(buf, "[Server] Vous allez être déconnecté...\n")==0){
      printf("              %s", buf);
      close (*(int *)arg);
      printf("              Connection terminée\n");
      break;
    }
    else if(strncmp(buf, "/send ",6)==0){
      pthread_t pthread3[2];
      pthread_create(&pthread3[1], NULL, start_read_send, (void*)&buf); //Create a thread to read a the file which is going to be send
    }
    else if(strcmp(buf, " ")==0){

    }
    else{
      printf ("             %s\n",buf);
    }
  }
  exit(12);
}


//send a request to the server in order to know the adress of the client we are looking for
int start_send(int sockfd, char *name, char *nom_fichier){
  int len=100;
  char message2[100];
  char message[100];
  memset(message2, 0, 100);
  strcat(message2, "/send ");
  strcat(message2, name);
  strcat(message2, " ");
  strcat(message2, nom_fichier);
  strcat(message2, " \n");
  __sync_synchronize();
  do_write(sockfd, message2, len);
  __sync_synchronize();
  pthread_mutex_lock(&verrou2);
  do_read(sockfd, message, len);
  __sync_synchronize();
  pthread_mutex_unlock(&verrou2);
  if(strcmp(message, "[Server] Ce client n'existe pas\n")==0){
    printf("              %s", message);
  }
  else if(strcmp(message, "[Server] Requête envoyée\n")==0){
    pthread_t pthread2[2];
    pthread_create(&pthread2[1], NULL, send_folder, (void*)nom_fichier);

  }
}




int main(int argc,char** argv)
{
  if (argc != 3)
  {
    fprintf(stderr,"              usage: RE216_CLIENT hostname port\n");
    return 1;
  }



  else {
    //get address info from the server
    struct sigaction sig;
    memset(&sig, '0', sizeof(sig));
    sig.sa_handler = traitant;
    sigaction(SIGINT, &sig, NULL);
    int sockfd;
    struct addrinfo *res, hints;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    getaddrinfo(argv[1], NULL, &hints, &res);
    if(res->ai_family==AF_INET6){
      struct sockaddr_in6 sin;
      sin.sin6_port=htons(atoi(argv[2]));
      sin.sin6_family=AF_INET6;
      inet_pton(AF_INET6, argv[1], &sin.sin6_addr);
      sockfd = do_socket(res->ai_family,SOCK_STREAM,0);
      do_connect6(sockfd,sin);
    }
    else if(res->ai_family==AF_INET){
      struct sockaddr_in sin;
      sin.sin_port=htons(atoi(argv[2]));
      sin.sin_family=AF_INET;
      inet_aton(argv[1], &sin.sin_addr);
      sockfd = do_socket(res->ai_family,SOCK_STREAM,0);
      do_connect(sockfd,sin);
    }


    //get the socket
    int len=100;
    char con_message[100];
    do_read(sockfd,con_message,len);
    if(strcmp(con_message,"[Server] Le serveur n'accepte plus de connexions\n")==0){
      printf("%s\n", con_message);
      close (sockfd);
      exit(0);
    }
    printf("Entrer un message: ");

    pthread_t pthread[2];
    pthread_create(&pthread[1], NULL, start_read, (void*)&sockfd);
    //connect to remote socket
    while(1){
      char message[100];
      pthread_mutex_lock(&verrou);
      fgets(message, len, stdin);
      pthread_mutex_unlock(&verrou);
      sleep(0.1);

      if (strncmp(message,"/send ",6)==0){
        char* name=strtok(message+6, " ");
        char* nom_fichier=strtok(NULL, "\n");
        FILE *fichier=NULL;
        fichier= fopen(nom_fichier,"r");
        if(fichier!=NULL){
          //Call the function for send the file
          start_send(sockfd, name, nom_fichier);
          fclose(fichier);
        }
        else{
          printf("                  Ce fichier n'existe pas \n");
        }
      }
      else if(strcmp(message,"\n")==0){

      }
      else{
        __sync_synchronize();
        do_write(sockfd, message, len);
        __sync_synchronize();
      }
    }
  }
  exit(0);
}
