#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#define MAXDATASIZE 256
#define READ  0
#define WRITE 1
void CheckCommand(int argc,const char**);//function for checking given command
void ParseCommand(const char**,int*,char*,char*);
bool CheckCredentials(int,char*,char*);
bool SendACK(int);
int ReceiveString(int,char*);
void StartSession(int);
char* SplitPipe(char*);
char** CommandToArgs(char*);
char* skipwhite(char*);
int RunCmd(int,char*,int,int,int);
int command(char**,int,int,int,int);
int ExecLine(char*,int);
void Mywait(int);

int main(int argc, char const *argv[]) {
  int MyPort;
  char User[MAXDATASIZE];
  char Pass[MAXDATASIZE];
  int sockfd,new_fd;
  struct sockaddr_in my_addr;
  struct sockaddr_in their_addr;
  socklen_t sin_size;
  bool CredBool;
  CheckCommand(argc,argv);
  ParseCommand(argv,&MyPort,User,Pass);
  if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) {//For tcp/ip SOCK_STREAM
    perror("socket");
    exit(-1);
  }
  my_addr.sin_family =AF_INET;//default for ipv4
  my_addr.sin_port = htons(MyPort);//port number with network byte order
  my_addr.sin_addr.s_addr= INADDR_ANY;//0.0.0.0 any address
  memset(&(my_addr.sin_zero),'\0',8);
  if ( bind(sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1 ) {
    perror("bind");
    exit(-1);
  }

  if (listen(sockfd, 10 == -1)) {
    perror("listen");
    exit(-1);
  }


  while (1) {// new accept
    sin_size =sizeof(struct sockaddr_in);
    if((new_fd =accept(sockfd,(struct sockaddr *)&their_addr,&sin_size)) == 0){
      perror("accept");
      continue;
    }
    CredBool=CheckCredentials(new_fd,User,Pass);

    if(CredBool==true)
    {
      printf("server : got connection from %s\n",inet_ntoa(their_addr.sin_addr));
      StartSession(new_fd);
    }
    printf("server : connection from %s is over\n",inet_ntoa(their_addr.sin_addr));
    close(new_fd);

  }

  return 0;
}

void StartSession(int sockfd) {
  char command[MAXDATASIZE];
  int numbytes=1;
  int exitbool=0;
  do {
    numbytes=ReceiveString(sockfd,command);
    if(numbytes != 0)
    {
      exitbool=ExecLine(command,sockfd);
    }
  } while(numbytes != 0 && exitbool != -1);
}

int ExecLine(char* command,int sockfd) {//for pipe commands
  int input=0;
  char* cmd=command;
  int first=1;
  char* next;
  int n=0;
  next=SplitPipe(command);
  while (next!=NULL && input != -1) {//if next is null, cmd points begining of previous command
    next[0]='\0';                      //if input == -1 call exit from client

    //printf("%s\n",cmd);
    input=RunCmd(input,cmd,first,0,sockfd);
    cmd=next+1;//cmd to next command after pipe
    next=SplitPipe(cmd);//find loc of pipe
    first=0;
    n++;
  }
  n++;
  input=RunCmd(input,cmd,first,1,sockfd);//input is file descriptor of before command stdout
  Mywait(n);
  if(input==-1)
  {
    return -1;
  }
 return 0;
}

int RunCmd(int input,char* cmd, int first, int last,int sockfd){
  char** args=NULL;
  args=CommandToArgs(cmd);
  while (args[0]!=NULL) {
    if (strcmp(args[0], "exit") == 0)
    {
      return -1;
    }
    return command(args,input,first,last,sockfd);
  }
  	return 0;
}




char** CommandToArgs(char* cmd) {
  cmd=skipwhite(cmd);
  int i=0;
  char** args =(char**) calloc(6,sizeof(char*));
  char* next= strchr(cmd,' ');
  while (next!=NULL) {
    next[0]='\0';
    args[i]=cmd;
    //printf("%s\n",args[i]);
    cmd=skipwhite(next+1);
    next=strchr(cmd,' ');
    i++;
  }
  if(cmd[0]!='\0')//
  {
    args[i]=cmd;
    next=strchr(cmd,'\n');
    next[0]='\0';
    i++;
  }
  args[i]=NULL;
  return args;
}

int command(char** args,int input,int first,int last,int sockfd){
  pid_t pid;
  int pipettes[2];
  pipe(pipettes);
  pid=fork();

  	if (pid == 0) {
  		if (first == 1 && last == 0 && input == 0) {// First command
  			dup2( pipettes[WRITE], STDOUT_FILENO );
  		} else if (first == 0 && last == 0 && input != 0) {// Middle command
  			dup2(input, STDIN_FILENO);
  			dup2(pipettes[WRITE], STDOUT_FILENO);
  		} else {// Last command
  			dup2( input, STDIN_FILENO);
        dup2( sockfd, STDOUT_FILENO);//After Last Command Exec Stdout To Socket
  		}
  		if (execvp(args[0], args) == -1)
  			_exit(EXIT_FAILURE); // If child fails
  	}
    if (input != 0)
  		close(input);

  	// Nothing more needs to be written
  	close(pipettes[WRITE]);

  	// If it's the last command, nothing more needs to be read
  	//if (last == 1)
  	//	close(pipettes[READ]);

  	return pipettes[READ];
}

char* SplitPipe(char* command ) {
  const char pipe = '|';
  char* next;
  next=strchr(command,pipe);
  return next;
}
char* skipwhite(char* command) {
  if(command[0]==' ')
  {
      command++;
  }
  return command;

}

bool CheckCredentials(int new_fd,char* User, char* Pass) {
  char bufferUser[MAXDATASIZE];
  char bufferPass[MAXDATASIZE];
  ReceiveString(new_fd,bufferUser);
  ReceiveString(new_fd,bufferPass);
  if (strcmp(bufferUser,User)!=0 || strcmp(bufferPass,Pass)!=0) {
    close(new_fd);
    return false;
  }
  else
  {
    SendACK(new_fd);
  }
  return true;
}

int ReceiveString(int sockfd,char* buffer){
  int numbytes;
  char buffer2[MAXDATASIZE];
  numbytes=recv(sockfd,buffer2,MAXDATASIZE-1,0);
  bool bool1=SendACK(sockfd);
  if(bool1==true)
  strcpy(buffer,buffer2);
  return numbytes;
}

bool SendACK(int sockfd){
  if (send(sockfd,"#",2,0)== -1) {
    return false;
  }
  return true;
}

void ParseCommand(const char* argv[],int* PortNumber,char* User, char* Pass) {
  *PortNumber=atoi(argv[2]);
  strcpy(User,argv[4]);
  strcpy(Pass,argv[6]);
}

void CheckCommand(int argc,const char* argv[]) {
  char PortArg[3]="-p";
  char UserArg[3]="-u";
  char PassArg[3]="-p";
  if (argc == 7) {
    if (strcmp(argv[1],PortArg)!=0){//Check Port Argument
      printf("\"%s\" is invalid option\n",argv[1]);
      exit(-1);
      }
    if (strcmp(argv[3],UserArg)!=0){//Check User Argument
      printf("\"%s\" is invalid option\n",argv[3]);
      exit(-1);
      }
    if (strcmp(argv[5],PassArg)!=0){//Check Password Argument
      printf("\"%s\" is invalid option\n",argv[5]);
      exit(-1);
      }

  }
  else{
    printf("usage: \"-p [PORTNUMBER] -u [USERNAME] -p [PASSWORD]\"\n" );
    exit(-1);
  }
}

void Mywait(int n) {
  int i;
  for (i = 0; i < n; i++) {
    wait(NULL);
  }
}
