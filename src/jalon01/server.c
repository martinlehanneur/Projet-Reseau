#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/ipc.h>
#include<fcntl.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <time.h>

void error(const char *msg)
{
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

void do_bind(int sockfd,struct sockaddr_in sin){
  if (bind(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0 ){
    error("Server : bind");
    exit (1);
  }
}

void do_listen(int sockfd, int nb_connexions){
  if (listen(sockfd,nb_connexions)<0){
    error("Server : listen");
    exit (1);
  }
}

int do_accept(int sockfd,struct sockaddr_storage sinclient){
  socklen_t len;
  len = sizeof sinclient;
  int var = accept(sockfd,(struct sockaddr*)&sinclient,&len);
  if(var<0){
    error("server: accept");
  }
  return var;
}

void do_read(int connexion, char *buf, int len){
  //  int len=sizeof(buf);
  if(read(connexion,buf,len)<0){
    error("server: read");
  }
}

void do_write(int connexion, char *buf, int len){
  //  int len=sizeof(buf);
  if(write(connexion,buf,len)<0){
    error("server: write");
  }
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: RE216_SERVER port\n");
    return 1;
  }

  int nb_connexions=5;

  struct sockaddr_in sin;
  struct pollfd fds[nb_connexions+1];
  sin.sin_family=AF_INET;
  sin.sin_port=htons(atoi(argv[1]));
  sin.sin_addr.s_addr=htonl (INADDR_ANY);
  int sockfd = do_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  int len_nom=20;
  struct client
  {
    char nom[len_nom];
    char adresse[100];
    char date[64];
    int port;
  };

  struct client tableau[nb_connexions+1];
  fds[0].fd=sockfd;
  fds[0].events=POLLIN;
  for(int j=1; j<=nb_connexions; j++){
    fds[j].fd=0;
    tableau[j].nom[0]='\0';
  }

  int len=100;
  char buf[len];
  struct sockaddr_storage sinclient;
//  socklen_t* addrlen= (socklen_t*)sizeof(sinclient);

  do_bind(sockfd, sin);
  do_listen(sockfd, nb_connexions+1);

  while(1){
    poll(fds, nb_connexions,0);
    for(int i=0; i<=nb_connexions; i++){
      if(fds[i].revents==POLLIN){
        if(fds[i].fd==sockfd){
          int connexion=do_accept(sockfd, sinclient);
          for(int k=0; k<nb_connexions; k++){
            if (fds[k].fd==0){
              struct sockaddr_in *s = (struct sockaddr_in *)&sinclient;
    tableau[k].port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, tableau[k].adresse, sizeof tableau[k].adresse);
              //tableau[k].adresse=inet_ntoa(sinclient.sin_addr);
              time_t t = time(NULL);
                  struct tm *tm = localtime(&t);
                  strftime(tableau[k].date, sizeof(tableau[k].date), "%c", tm);
              fds[k].fd=connexion;
              fds[k].events=POLLIN;
              do_write(fds[k].fd,"Vous êtes maintenant connecté",len);
              break;
            }
            else if (k==(nb_connexions-1) && fds[k].fd!=0){
              fds[nb_connexions].fd=connexion;
              fds[nb_connexions].events=POLLIN;
              do_write(fds[nb_connexions].fd,"Le serveur n'accepte plus de connexions\n",len);
              close(fds[nb_connexions].fd);
            }
          }
        }
        else {
          do_read(fds[i].fd, buf, len);

          if(strcmp(buf, "/quit\n")==0){
            do_write(fds[i].fd,"Vous allez être déconnecté",len);
            close(fds[i].fd);
            fds[i].fd=0;
            tableau[i].nom[0]='\0';
          }

          else if (tableau[i].nom[0]=='\0' && strncmp(buf,"/nick ", 6)!=0){
            do_write(fds[i].fd,"Vous devez entrer un nom: /nick nom",len);
          }

          else if(tableau[i].nom[0]=='\0' && strncmp(buf,"/nick ", 6)==0){
            int j=6;
            while(j<26){
              tableau[i].nom[j-6]=buf[j];
              j++;
            }
            char message[1000]="Bonjour, bienvenue à toi:";
            strcat(message, tableau[i].nom);
            do_write(fds[i].fd, message, len);
          }

          else if(strncmp(buf,"/nick ", 6)==0){
            int j=6;
            int var=6;
            while(j<(len_nom+var)){
              tableau[i].nom[j-var]=buf[j];
              j++;
            }
            char message[1000]="Ton nouveau nom est : ";
            strcat(message, tableau[i].nom);
            do_write(fds[i].fd, message, len);
          }

          else if (strcmp(buf,"/who\n")==0){
            char message[1000]="Online users are:\n";
            int j=1;
            while(j<nb_connexions){
              if(tableau[j].nom[0]!='\0'){
                strcat(message, " - ");
                strcat(message, tableau[j].nom);
              }
              j++;
            }
            do_write(fds[i].fd,message,1000);
          }

          else if (strncmp(buf,"/whois ",6)==0){
            char message[1000]="";
            char nom[20]="";
            int j=0;
            int var=7;
            while(j<20){
              nom[j]=buf[j+var];
              j++;
            }
            j=1;
            while(j<nb_connexions){
              if(strcmp(tableau[j].nom, nom)==0){
                char port[sizeof(int)];
                strcat(message, tableau[j].nom);
                strcat(message," connected since ");
                strcat(message, tableau[j].date);
                strcat(message," with IP address ");
                strcat(message, tableau[j].adresse);
                strcat(message," and port number ");
                sprintf(port, "%d", tableau[j].port);
                strcat(message, port);
                break;
              }
              j++;
              if(j==(nb_connexions-1)){
                strcat(message, "Ce client n'existe pas !");
              }
            }
            do_write(fds[i].fd,message,1000);
          }

          else{
            do_write(fds[i].fd,buf,len);
          }
        }
      }
    }

  }
  //close (sockfd);
  exit (0);
}
