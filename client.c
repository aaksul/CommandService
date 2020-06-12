#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#define MAXDATASIZE 256
void CheckCommand(int argc,const char**);//function for checking given command
void ParseCommand(const char**,char*,int*,char*,char*);
void SendCredentials(int,char*,char*);
bool receiveAck(int);
bool SendMsg(int,char*);
void StartSession(int);
void SendCommandToServer(int,char*);
void recvCommandOutput(int);
int main(int argc, char const *argv[]) {
  int PORT;
  char hostname[12];
  char User[12];
  char Pass[12];
  int sockfd;
  struct hostent *he;
  struct sockaddr_in their_addr;
  CheckCommand(argc,argv);
  ParseCommand(argv,hostname,&PORT,User,Pass);

  if ((he=gethostbyname(hostname))==NULL) {//get the host info
    perror("gethostbyname");
    exit(1);
  }
  if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
    perror("socket");
    exit(1);
  }

  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(PORT);
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(their_addr.sin_zero),'\0',8);


  if (connect(sockfd,(struct sockaddr*)&their_addr,sizeof(struct sockaddr))==-1) {
    perror("connect");
    exit(1);
  }

  SendCredentials(sockfd,User,Pass);
  StartSession(sockfd);


  return 0;
}

void StartSession(int sockfd) {
  char line[MAXDATASIZE];
  while (1) {
    printf("$>");
		fflush(NULL);
		if (!fgets(line, MAXDATASIZE, stdin))
      perror("fgets");
    SendCommandToServer(sockfd,line);
    recvCommandOutput(sockfd);
  }
  close(sockfd);
}
void SendCredentials(int sockfd,char* User, char* Pass) {
  int numbytes;
  char buffer[MAXDATASIZE];
  SendMsg(sockfd,User);
  SendMsg(sockfd,Pass);
  numbytes=recv(sockfd,buffer,MAXDATASIZE-1,0);
  if(numbytes==0){
    printf("Wrong Username or Password\n");
    exit(-1);
  }
}


void recvCommandOutput(int sockfd) {
  char buffer[1024];
  int numbytes;
  numbytes=recv(sockfd,buffer,1023,0);
  if (numbytes==0) {
    close(sockfd);
    exit(-1);
  }
  printf("%s\n",buffer);
}


void SendCommandToServer(int sockfd,char* command) {
    SendMsg(sockfd,command);
}


bool SendMsg(int sockfd,char* string){
  bool bool1;
  send(sockfd,string,MAXDATASIZE-1,0);
  bool1=receiveAck(sockfd);
  if (bool1==true) {
    return true;
  }
  return false;
}

bool receiveAck(int sockfd){
  char buffer[2];
  recv(sockfd,buffer,2,0);
  if (strcmp(buffer,"#")==0) {
    return true;
  }
  return false;
}

void ParseCommand(const char* argv[],char* hostname,int* PortNumber,char* User, char* Pass ) {
  strcpy(hostname,argv[2]);
  *PortNumber=atoi(argv[4]);
  strcpy(User,argv[6]);
  strcpy(Pass,argv[8]);
}

void CheckCommand(int argc,const char* argv[]) {
  char HostArg[3]="-h";
  char PortArg[3]="-p";
  char UserArg[3]="-u";
  char PassArg[3]="-p";
  if (argc == 9) {
    if (strcmp(argv[1],HostArg)!=0){//Check Port Argument
      printf("\"%s\" is invalid option\n",argv[1]);
      exit(-1);
      }
    if (strcmp(argv[3],PortArg)!=0){//Check Port Argument
      printf("\"%s\" is invalid option\n",argv[3]);
      exit(-1);
      }
    if (strcmp(argv[5],UserArg)!=0){//Check User Argument
      printf("\"%s\" is invalid option\n",argv[5]);
      exit(-1);
      }
    if (strcmp(argv[7],PassArg)!=0){//Check Password Argument
      printf("\"%s\" is invalid option\n",argv[7]);
      exit(-1);
      }

  }
  else{
    printf("usage: \"-h [HOSTNAME] -p [PORTNUMBER] -u [USERNAME] -p [PASSWORD]\"\n" );
    exit(-1);
  }
}
