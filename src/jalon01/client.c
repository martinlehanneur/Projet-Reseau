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

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;



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

void start_read_send(int sockfd, char buf[]){
  char message[100]="";
  char message2[100]="";
  char message3[100]="";
  char message4[100]="";
  char message5[100]="";
  char* nom_envoyeur=strtok(buf+6, " ");
  char* nom_fichier_envoye=strtok(NULL, "\n");
  int len=100;
  char* comparaison="              [";
  strcat(message, nom_envoyeur);
  strcat(message, " vous envoie le fichier ");
  strcat(message, nom_fichier_envoye);
  strcat(message, "\nAcceptez vous le fichier [y/n]\n");
  strcat(message2, "/msg ");
  strcat(message2, nom_envoyeur);
  strcat(message2, " Fichier accepté\n");
  strcat(message3, "/msg ");
  strcat(message3, nom_envoyeur);
  strcat(message3, " Fichier transféré\n");
  strcat(message4, nom_fichier_envoye);
  strcat(message4, " sauvegardé dans /Projet-Reseau/");
  strcat(message4, nom_fichier_envoye);
  strcat(message5, "/msg ");
  strcat(message5, nom_envoyeur);
  strcat(message5, " Fichier refusé\n");
  printf("%s", message);
  while(1){
    char reponse[100];
    fgets(reponse, len, stdin);
    if(strncmp(reponse, "y",1)==0 ){
      FILE* fichier=NULL;
      fichier=fopen("../../texte.txt","a");
      __sync_synchronize();
      do_write(sockfd, message2, len);
      __sync_synchronize();
      while(1){
        char buf2[1000];
        __sync_synchronize();
        do_read(sockfd,buf2,1000);
        __sync_synchronize();
        if(strncmp(buf2, "/send2 ", 7)==0){
          strtok(buf2+7, " ");
          char* texte=strtok(NULL, "\n");
          fputs(texte, fichier);
          fputs("\n", fichier);
        }
        else if(strncmp(buf2, comparaison, 15)==0){
          break;
        }

      }
      fclose(fichier);
      __sync_synchronize();
      printf("%s\n", message4);
      do_write(sockfd, message3, len);
      __sync_synchronize();
      break;

    }
    else if (strncmp(reponse, "n", 1)==0){
      __sync_synchronize();
      do_write(sockfd, message5, len);
        __sync_synchronize();
      break;
    }
  }
}


void* start_read(void *arg){
  while(1){
    char buf[1000];
    char message[100]="";
      __sync_synchronize();
    do_read(*(int *)arg,buf,1000);
    __sync_synchronize();
    if(strcmp(buf, "              [Server] Vous allez être déconnecté...\n")==0){
      printf("%s", buf);
      close (*(int *)arg);
      printf("Connection terminée\n");
      break;
    }
    else if(strncmp(buf, "/send ",6)==0){
      pthread_mutex_lock(&verrou);
      start_read_send(*(int *)arg, buf);
      pthread_mutex_unlock(&verrou);
    }
    else{
      printf ("%s\n",buf);
    }
  }
  exit(12);
}

void start_send(int sockfd, char *name, char *nom_fichier, FILE* fichier){
  char buf[1000];
  int len=100;
  char message3[100]="";
  strcat(message3,"/msg ");
  strcat(message3, name);
  strcat(message3, " Fichier tranféré\n");
    __sync_synchronize();
  do_read(sockfd,buf,1000);
  __sync_synchronize();
  char* nom=strtok(buf, "]");
  char* message_recu=strtok(NULL, "\n");
  if(strcmp("Fichier accepté", message_recu)==0){
    int taille_max=70;
    char texte[taille_max];
      while(fgets(texte, taille_max, fichier)!=NULL){
        char message2[100]="";
        strcat(message2,"/send2 ");
        strcat(message2, name);
        strcat(message2, " ");
        strcat(message2, texte);
        __sync_synchronize();
        do_write(sockfd, message2, len);
          __sync_synchronize();
      }
      __sync_synchronize();
      do_write(sockfd, message3, len);
        __sync_synchronize();
    }
    else{
      printf("                L'utilisateur renseigné n'existe pas\n");
      fflush(stdin);
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
          __sync_synchronize();
          do_write(sockfd, message, len);
          __sync_synchronize();
          start_send(sockfd, name, nom_fichier, fichier);
        }
        else{
          printf("                  Ce fichier n'existe pas \n");
          fflush(stdin);
        }
fclose(fichier);
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
