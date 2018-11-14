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
#include <pthread.h>

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

typedef struct User User;
struct User
{
  char* name;
  int sockfd;
  User *next;
};


typedef struct Salon Salon;
struct Salon
{
  char* name;
  Salon *next;
  User *first;
};

typedef struct AllSalon AllSalon;
struct AllSalon
{
  Salon *next;
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

AllSalon *initialisation_allsalon()
{
  AllSalon *AllSalon = malloc(sizeof(*AllSalon));
  if (AllSalon == NULL)
  {
    exit(EXIT_FAILURE);
  }

  AllSalon->next = NULL;
  return AllSalon;
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

void insertion_salon(AllSalon *allsalon, char* name)
{
  Salon *nouveau = malloc(sizeof(*nouveau));
  if (allsalon == NULL || nouveau == NULL)
  {
    exit(EXIT_FAILURE);
  }
  nouveau->name=malloc(20*sizeof(char));
  nouveau->next = allsalon->next;
  allsalon->next = nouveau;
  strcpy(nouveau->name,name);
  nouveau->first=NULL;

}

void insertion_user(Salon *salon, char* name, int sockfd)
{
  User *nouveau = malloc(sizeof(*nouveau));
  if (salon == NULL || nouveau == NULL)
  {
    exit(EXIT_FAILURE);
  }
  nouveau->name=malloc(20*sizeof(char));
  nouveau->next = salon->first;
  nouveau->sockfd = sockfd;
  salon->first = nouveau;
  strcpy(nouveau->name,name);

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

void delete_salon(AllSalon *allsalon, char* name)
{
  Salon *tmp;
  Salon *previous;
  tmp=allsalon->next;
  previous=allsalon->next;
  if (tmp == NULL)
  {
    exit(EXIT_FAILURE);
  }
  else if(strcmp(tmp->name,name)==0){
    allsalon->next=tmp->next;
  }
  else{
    while (tmp != NULL)
    {
      if(strcmp(tmp->name,name)==0){
        previous->next = tmp->next;
        break;
      }
      previous=tmp;
      tmp=tmp->next;
    }
  }
  free(tmp->name);
  free(tmp);
}

int delete_user(Salon *salon, int sockfd)
{
  User *tmp;
  User *previous;
  tmp=salon->first;
  previous=salon->first;
  if (tmp == NULL)
  {
    return 0;
  }
  else if(sockfd==tmp->sockfd){
    salon->first=tmp->next;
    free(tmp->name);
    free(tmp);
    return 1;
  }
  else{
    while (tmp != NULL)
    {
      if(sockfd==tmp->sockfd){
        previous->next = tmp->next;
        free(tmp->name);
        free(tmp);
        return 1;
      }
      previous=tmp;
      tmp=tmp->next;
    }
  }
  return 0;
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

int do_accept(int sockfd,struct sockaddr_in sinclient){
  socklen_t len;
  len = sizeof(sinclient);
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

void* start_read(void *arg){
  while(1){
    char buf[1000];
    char message[100]="";
    __sync_synchronize();
    fgets(message, 100, stdin);
    __sync_synchronize();

    if(strcmp(message,"/quit\n")==0){



      Element* element;
      List *client=(List *)arg;
      element=client->first;
      char message[1000]="";
      if(element==NULL){
        exit(14);
      }
      while(client->first != NULL)
      {
        do_write(client->first->sockfd,"[Server] Vous allez être déconnecté...\n",100);
        close(client->first->sockfd);
        delete(client,client->first->sockfd);
        if(client->first==NULL){
          exit(13);
        }
      }
    }

    else{
      printf ("             \n/quit if you want to quit\n");
    }
  }
  exit(12);
}

void traitant(int a){
  printf("\n              /quit if you want to quit\n ");

}



int main(int argc, char** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: RE216_SERVER port\n");
    return 1;
  }

  struct sigaction sig;
  memset(&sig, '0', sizeof(sig));
  sig.sa_handler = traitant;
  sigaction(SIGINT, &sig, NULL);

  int nb_connexions=5;
  int len_name=20;
  int len=100;
  char *buf=malloc(len*sizeof(char));
  char adresse[30];
  char temps[30];

  struct sockaddr_in6 sin;
  struct sockaddr_in6 sinclient={0};
  sin.sin6_family=AF_INET6;
  sin.sin6_port=htons(atoi(argv[1]));
  sin.sin6_addr=in6addr_any;
  int sockfd = do_socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);


  struct pollfd fds[nb_connexions+2];
  fds[0].fd=sockfd;
  fds[0].events=POLLIN;
  for(int j=1; j<=nb_connexions; j++){
    fds[j].fd=0;
  }

  List *client = initialisation();
  AllSalon *allsalon = initialisation_allsalon();

  pthread_t pthread[2];
  pthread_create(&pthread[1], NULL, start_read, (void*)client);

  do_bind(sockfd, sin);
  do_listen(sockfd, nb_connexions+1);

  while(1){
    poll(fds, nb_connexions+1,0);
    for(int i=0; i<=nb_connexions; i++){
      if(fds[i].revents==POLLIN){

        if(fds[i].fd==sockfd){
          socklen_t len;
          len = sizeof(sinclient);
          int connexion = accept(sockfd,(struct sockaddr*)&sinclient,&len);
          //int connexion=do_accept(sockfd, sinclient);
          if (length(client)<nb_connexions){
            struct tm *tm= malloc(sizeof(*tm));
            char adresse[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &sinclient.sin6_addr, adresse, sizeof(adresse));
            //strcpy(adresse,inet_ntoa(sinclient.sin6_addr));
            //printf("adresse=%s\n",inet_ntoa(sinclient.sin6_addr));
            time_t t = time(NULL);
            tm=localtime(&t);
            strftime(temps, sizeof(temps), "%c", tm);

            insertion(client, adresse, temps, ntohs(sinclient.sin6_port), connexion);

            for(int k=1; k<=nb_connexions; k++){
              if(fds[k].fd==0){
                fds[k].fd=connexion;
                fds[k].events=POLLIN;
                do_write(fds[k].fd,"[Server] Vous êtes maintenant connecté",len);
                break;
              }
            }
          }
          else {
            fds[nb_connexions+1].fd=connexion;
            fds[nb_connexions+1].events=POLLIN;
            do_write(fds[nb_connexions+1].fd,"[Server] Le serveur n'accepte plus de connexions\n",len);
            close(fds[nb_connexions+1].fd);
          }
        }

        else {
          do_read(fds[i].fd, buf, len);
          Element *current_client=find_client(client, fds[i].fd);

          if(strcmp(buf, "/quit\n")==0){
            Salon* salon;
            salon=allsalon->next;
            char message[1000]="";
            while(salon != NULL)
            {
              delete_user(salon, fds[i].fd);
              if(salon->first==NULL){
                strcat(message, "[");
                strcat(message, salon->name);
                strcat(message, "] Le salon a été supprimé\n");
                delete_salon(allsalon, salon->name);
                do_write(fds[i].fd,message,len);
              }
              salon = salon->next;
            }
            do_write(fds[i].fd,"[Server] Vous allez être déconnecté...\n",len);
            delete(client,fds[i].fd);
            close(fds[i].fd);
            fds[i].fd=0;
          }

          else if (strcmp(current_client->name,"\0")==0 && strncmp(buf,"/nick ", 6)!=0){
            do_write(fds[i].fd,"[Server] Vous devez entrer un name: /nick name",len);
          }

          else if(strcmp(current_client->name,"\0")==0 && strncmp(buf,"/nick ", 6)==0){
            char message[100]="";
            if(strstr(buf+6, " ")==NULL && buf[7]!='\0'){
              strcpy(current_client->name,buf+6);
              current_client->name[strlen(current_client->name)-1]=0;
              strcat(message,"[Server] Bonjour, bienvenue à toi: ");
              strcat(message, buf+6);
              do_write(fds[i].fd, message, len);
            }
            else{
              do_write(fds[i].fd, "[Server] Nom Invalide: Espaces interdits dans votre nom\n", len);
            }
          }

          else if(strncmp(buf,"/nick ", 6)==0){
            int j=6;
            int var=6;
            char message[100]="";
            if(strstr(buf+6, " ")==NULL && buf[7]!='\0'){
              strcpy(current_client->name,buf+6);
              current_client->name[strlen(current_client->name)-1]=0;
              strcat(message,"[Server] Ton nouveau nom est : ");
              strcat(message, buf+6);
              do_write(fds[i].fd, message, len);
            }
            else{
              do_write(fds[i].fd, "[Server] Nom Invalide: Espaces interdits dans votre nom\n", len);
            }
          }

          else if (strcmp(buf,"/who\n")==0){
            char message[100]="";
            Element* element;
            element=client->first;
            strcat(message,"[Server] Online users are:");
            while(element != NULL)
            {
              strcat(message, "\n - ");
              strcat(message, element->name);
              element = element->next;
            }
            do_write(fds[i].fd,message,1000);
          }

          else if (strncmp(buf,"/whois ",7)==0){
            char message[1000]="";
            Element *researched_client;
            char *name=strtok(buf+7, "\n");
            researched_client=find_client_name(client, name);
            if(researched_client!=NULL){
              char port[sizeof(int)];
              strcat(message,"[Server] ");
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
              strcat(message, "[Server] Ce client n'existe pas !");
            }
            do_write(fds[i].fd,message,1000);
          }
          else if (strncmp(buf,"/msgall ",8)==0){
            char message[1000]="";
            Element* element;
            element=client->first;
            strcat(message,"[");
            strcat(message,current_client->name);
            strcat(message,"]");
            strcat(message, buf+8);
            while(element != NULL)
            {
              if(element->sockfd!=fds[i].fd){
                do_write(element->sockfd, message,1000);
              }
              element = element->next;
            }

          }
          else if (strncmp(buf,"/msg ",5)==0){
            char message[1000]="";
            char *name=strtok(buf+5, " ");
            Element* element;
            element=client->first;
            strcat(message,"[");
            strcat(message,current_client->name);
            strcat(message,"] ");
            char* recu=strtok(NULL, "\n");
            strcat(message,recu);
            while(element != NULL)
            {
              if(strcmp(element->name,name)==0){
                do_write(element->sockfd, message,1000);
                break;
              }
              if(element->next==NULL){
                do_write(fds[i].fd, "[Server] Ce destinataire est inconnu\n",1000);
                break;
              }
              element = element->next;
            }
          }
          else if (strncmp(buf,"/create ",8)==0){
            char *name=strtok(buf+8, "\n");
            Salon* salon;
            if(allsalon->next==NULL){
              insertion_salon(allsalon,name);
              char message[1000]="";
              strcat(message, "[Server] Vous avez créé le salon ");
              strcat(message, name);
              strcat(message, "\n");
              do_write(fds[i].fd,message,1000);
              salon=NULL;
            }
            else{
              salon=allsalon->next;
            }
            while(salon != NULL)
            {
              char message[1000]="";
              if(strcmp(salon->name,name)==0){
                strcat(message, "[Server] Ce nom de salon existe déjà !\n");
                do_write(fds[i].fd,message,1000);
                break;
              }
              else if(salon->next==NULL){
                insertion_salon(allsalon, name);
                strcat(message, "[Server] Vous avez créé le salon ");
                strcat(message, name);
                strcat(message, "\n");
                do_write(fds[i].fd,message,1000);
                break;
              }
              else{
                salon = salon->next;
              }
            }
          }
          else if (strncmp(buf,"/join ",6)==0){
            char *name=strtok(buf+6, "\n");
            Salon* salon;
            salon=allsalon->next;
            char message[1000]="";
            if(salon==NULL){
              strcat(message, "[Server] Ce salon n'existe pas\n");
              do_write(fds[i].fd,message,1000);
            }
            while(salon != NULL)
            {
              if(strcmp(salon->name,name)==0){
                insertion_user(salon, name, fds[i].fd);
                strcat(message, "[");
                strcat(message,name);
                strcat(message, "] Bienvenu dans le salon !\n");
                do_write(fds[i].fd,message,1000);
                break;
              }
              else if(salon->next==NULL){
                strcat(message, "[Server] Ce salon n'existe pas\n");
                do_write(fds[i].fd,message,1000);
                break;
              }
              else{
                salon = salon->next;
              }
            }
          }
          else if (strncmp(buf,"/msgsalon ",10)==0){
            char message[1000]="";
            char *name=strtok(buf+10, " ");
            Salon* salon;
            if(allsalon->next==NULL){
              strcat(message,"[Server] Ce salon n'existe pas !\n");
              do_write(fds[i].fd, message,1000);
            }
            else{
              salon=allsalon->next;
            }

            while(salon != NULL)
            {
              if(strcmp(salon->name,name)==0){
                User* user;
                if(salon->first==NULL){
                  strcat(message,"[Server] Vous n'êtes pas membre de ce salon !\n");
                  do_write(fds[i].fd, message,1000);
                  break;
                }
                else{
                  user=salon->first;
                }
                while(user!=NULL){
                  if(fds[i].fd==user->sockfd){
                    strcat(message,"[");
                    strcat(message,salon->name);
                    strcat(message,"]");
                    strcat(message,"[");
                    strcat(message,current_client->name);
                    strcat(message,"]");
                    char* recu=strtok(NULL, "\n");
                    strcat(message,recu);
                    user=salon->first;
                    if(user->sockfd==fds[i].fd){
                      user=user->next;
                    }
                    while(user!=NULL){
                      do_write(user->sockfd, message,1000);
                      if(user->next==NULL){
                        break;
                      }
                      user=user->next;
                      if(user->sockfd==fds[i].fd){
                        user=user->next;
                      }
                    }
                    break;
                  }

                  else if(user->next==NULL){
                    strcat(message,"[Server] Vous n'êtes pas membre de ce salon\n");
                    do_write(fds[i].fd, message,1000);
                    break;
                  }
                  user=user->next;
                }

                break;
              }
              else if(salon->next==NULL){
                strcat(message,"[Server]Ce salon n'existe pas !\n");
                do_write(fds[i].fd, message,1000);
                break;
              }
              salon = salon->next;
            }
          }
          else if (strncmp(buf,"/quit ",6)==0){
            char message[1000]="";
            char *name=strtok(buf+6, "\n");
            Salon* salon;
            if(allsalon->next==NULL){
              strcat(message,"[server] Ce salon n'existe pas !\n");
              do_write(fds[i].fd, message,1000);
            }
            else{
              salon=allsalon->next;
            }

            while(salon != NULL)
            {
              if(strcmp(salon->name,name)==0){
                User* user;
                if(delete_user(salon, fds[i].fd)==0){
                  strcat(message,"[server] Vous n'êtes pas membre de ce salon !\n");
                  do_write(fds[i].fd, message,1000);
                  break;
                }
                else{
                  strcat(message,"[");
                  strcat(message,salon->name);
                  strcat(message,"] Vous avez quitté le salon !\n");
                  do_write(fds[i].fd, message,1000);
                  if(salon->first==NULL){
                    strcat(message, "[");
                    strcat(message, salon->name);
                    strcat(message, "] Le salon a été supprimé\n");
                    delete_salon(allsalon, salon->name);
                    do_write(fds[i].fd, message,1000);
                  }

                }
                break;
              }
              else if(salon->next==NULL){
                strcat(message,"[Server] Ce salon n'existe pas !\n");
                do_write(fds[i].fd, message,1000);
                break;
              }
              salon = salon->next;
            }
          }
          else if (strncmp(buf,"/send ",6)==0){
            char message[1000]="";
            char message2[1000]="";
            char* name=strtok(buf+6, " ");
            char* recu=strtok(NULL, "\n");
            Element* element;
            element=client->first;
            while(element != NULL)
            {
              if(strcmp(element->name,name)==0){
                char* adresse=current_client->adresse;
                strcat(message, "/send ");
                strcat(message, adresse);
                strcat(message, " ");
                strcat(message, current_client->name);
                strcat(message, " ");
                strcat(message, recu);
                strcat(message,"\n");
                do_write(fds[i].fd, " ",1000);
                  __sync_synchronize();
                do_write(fds[i].fd, "[Server] Requête envoyée\n",1000);
                do_write(element->sockfd, message,1000);

                break;
              }

              else if(element->next==NULL){
                do_write(fds[i].fd, " ",1000);
                do_write(fds[i].fd, "[Server] Ce client n'existe pas\n",1000);
                break;
              }
              element = element->next;
            }
          }

          else{
            char message[100]="";
            strcat(message, "[Server] No commande found: ");
            strcat(message, buf);
            do_write(fds[i].fd,message,len);
          }
        }
      }
    }

  }
  exit (0);
}
