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

typedef struct Element Element;
struct Element
{
  char* name;
  char* adresse;
  char* date;
  int port;
  int sockfd;
  Element *next;
};

typedef struct List List;
struct List
{
  Element *first;
};

List *initialisation()
{
  List *list = malloc(sizeof(*list));
  Element *element = malloc(sizeof(*element));
  if (list == NULL || element == NULL)
  {
    exit(EXIT_FAILURE);
  }

  list->first = NULL;
  return list;
}

void insertion(List *list, char* adresse, char* date, int port, int sockfd)
{
  Element *nouveau = malloc(sizeof(*nouveau));
  if (list == NULL || nouveau == NULL)
  {
    exit(EXIT_FAILURE);
  }
  nouveau->name=malloc(20*sizeof(char));
  nouveau->adresse=malloc(30*sizeof(char));
  nouveau->date=malloc(40*sizeof(char));
  nouveau->port=port;
  nouveau->sockfd = sockfd;
  nouveau->next = list->first;
  list->first = nouveau;
  strcpy(nouveau->date,date);
  strcpy(nouveau->name,"\0");
  strcpy(nouveau->adresse,adresse);

}

void delete(List *list, int sockfd)
{
  Element *tmp;
  Element *previous;
  tmp=list->first;
  previous=list->first;
  if (tmp == NULL)
  {
    exit(EXIT_FAILURE);
  }
  else if(tmp->sockfd==sockfd){
    list->first=tmp->next;
  }
  else{
    while (tmp != NULL)
    {
      if(tmp->sockfd==sockfd){
        previous->next = tmp->next;
        break;
      }
      previous=tmp;
      tmp=tmp->next;
    }
  }
  free(tmp->date);
  free(tmp->adresse);
  free(tmp->name);
  free(tmp);
}

void printList(List *list)
{
  if (list == NULL)
  {
    exit(EXIT_FAILURE);
  }
  Element *actual = list->first;
  while (actual != NULL)
  {
    printf("%s -> ", actual->name);
    actual = actual->next;
  }
  printf("NULL\n");
}


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
    error("Server : listn");
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
  if(read(connexion,buf,len)<0){
    error("server: read");
  }
}

void do_write(int connexion, char *buf, int len){
  if(write(connexion,buf,len)<0){
    error("server: write");
  }
}



int length(List *list)
{
  int n=0;
  Element *element=malloc(sizeof(*element));
  element=list->first;
  while(element!=NULL)
  {
    n++;
    element = element->next;
  }
  free(element);
  return n;
}

Element *find_client(List *list, int sockfd){
  Element *element;
  element=list->first;
  while(element!=NULL){
    if(element->sockfd==sockfd){
      return element;
    }
    element=element->next;
  }
  return NULL;
}

Element *find_client_name(List *list, char* name){
  Element *element;
  element=list->first;
  while(element!=NULL){
    if(strcmp(name, element->name)==0){
      return element;
    }
    element=element->next;
  }
  return NULL;
}



int main(int argc, char** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: RE216_SERVER port\n");
    return 1;
  }

  int nb_connexions=5;
  int len_name=20;
  int len=100;
  char *buf=malloc(len*sizeof(char));
  char adresse[30];
  char temps[30];

  struct sockaddr_in sin;
  struct sockaddr_storage sinclient;
  sin.sin_family=AF_INET;
  sin.sin_port=htons(atoi(argv[1]));
  sin.sin_addr.s_addr=htonl (INADDR_ANY);
  int sockfd = do_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

  struct pollfd fds[nb_connexions+2];
  fds[0].fd=sockfd;
  fds[0].events=POLLIN;
  for(int j=1; j<=nb_connexions; j++){
    fds[j].fd=0;
  }

  List *client = initialisation();

  do_bind(sockfd, sin);
  do_listen(sockfd, nb_connexions+1);

  while(1){
    poll(fds, nb_connexions+1,0);
    for(int i=0; i<=nb_connexions; i++){
      if(fds[i].revents==POLLIN){

        if(fds[i].fd==sockfd){
          int connexion=do_accept(sockfd, sinclient);
          if (length(client)<nb_connexions){
            struct sockaddr_in *s;
            struct tm *tm= malloc(sizeof(*tm));
            s=(struct sockaddr_in *)&sinclient;
            inet_ntop(AF_INET, &s->sin_addr, adresse, sizeof adresse);
            time_t t = time(NULL);
            tm=localtime(&t);
            strftime(temps, sizeof(temps), "%c", tm);
            insertion(client, adresse, temps, ntohs(s->sin_port), connexion);

            for(int k=1; k<=nb_connexions; k++){
              if(fds[k].fd==0){
                fds[k].fd=connexion;
                fds[k].events=POLLIN;
                do_write(fds[k].fd,"Vous êtes maintenant connecté",len);
                break;
              }
            }
          }
          else {
            fds[nb_connexions+1].fd=connexion;
            fds[nb_connexions+1].events=POLLIN;
            do_write(fds[nb_connexions+1].fd,"Le serveur n'accepte plus de connexions\n",len);
            close(fds[nb_connexions+1].fd);
          }
        }

        else {
          do_read(fds[i].fd, buf, len);
          Element *current_client=find_client(client, fds[i].fd);

          if(strcmp(buf, "/quit\n")==0){
            do_write(fds[i].fd,"Vous allez être déconnecté",len);
            delete(client,fds[i].fd);
            close(fds[i].fd);
            fds[i].fd=0;
          }

          else if (strcmp(current_client->name,"\0")==0 && strncmp(buf,"/nick ", 6)!=0){
            do_write(fds[i].fd,"Vous devez entrer un name: /nick name",len);
          }

          else if(strcmp(current_client->name,"\0")==0 && strncmp(buf,"/nick ", 6)==0){
            char message[100]="";
            strcpy(current_client->name,buf+6);
            strcat(message,"Bonjour, bienvenue à toi: ");
            strcat(message, buf+6);
            do_write(fds[i].fd, message, len);
          }

          else if(strncmp(buf,"/nick ", 6)==0){
            int j=6;
            int var=6;
            char message[100]="";
            strcpy(current_client->name,buf+6);
            strcat(message,"Ton nouveau name est : ");
            strcat(message, buf+6);
            do_write(fds[i].fd, message, len);
          }

          else if (strcmp(buf,"/who\n")==0){
            char message[100]="";
            Element* element;
            element=client->first;
            strcat(message,"Online users are:\n");
            while(element != NULL)
            {
              strcat(message, " - ");
              strcat(message, element->name);
              element = element->next;
            }
            do_write(fds[i].fd,message,1000);
          }

          else if (strncmp(buf,"/whois ",7)==0){
            char message[1000]="";
            Element *researched_client;
            researched_client=find_client_name(client, buf+7);
            if(researched_client!=NULL){
              char port[sizeof(int)];
              strcat(message, researched_client->name);
              strcat(message," connected since ");
              strcat(message, researched_client->date);
              strcat(message," with IP address ");
              strcat(message, researched_client->adresse);
              strcat(message," and port number ");
              sprintf(port, "%d", researched_client->port);
              strcat(message, port);
            }

            else{
              strcat(message, "Ce client n'existe pas !");
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
  exit (0);
}