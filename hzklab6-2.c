/*
* hzklab6-2.c
*
*  Created on: Apr 12, 2016
*      Author: zhqk6
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/mman.h>

unsigned long *VIC2SoftInt;
unsigned long *ptr;
int sock;
socklen_t fromlen;
int fd_fifo_out;
int fd_fifo_in;
struct sockaddr_in anybodyaddr;
char buffer2[10];
int R=2;

void error(const char *msg){
  perror(msg);
  exit(0);
}

void broadcast(){
  while(1){
    if((fd_fifo_out = open("/dev/rtf/1",O_RDWR))<0){    //open fifo from button event
      printf("open fifo error");
      exit(-1);
    }
    if(read(fd_fifo_out,buffer2,sizeof(buffer2))<0){
      printf("read fifo error");         //read fifo from button event
      exit(-1);
      //open and read FIFO from kernel which will transmit @A@B@C@D@E
    }
    if(R==1){
      anybodyaddr.sin_addr.s_addr=inet_addr("10.3.52.255");
      sendto(sock,buffer2,sizeof(buffer2),0,(struct sockaddr*)&anybodyaddr,fromlen);
      //send the messages which is read from FIFO to other slaves
      bzero(buffer2,sizeof(buffer2));
      fflush(stdout);
    }
  }
}

int main(int argc, char *argv[]){
  pthread_t broadcast_thread;

  struct sockaddr_in serv_addr;
  int port;
  int boolval=1;
  int fp;
  fp = open("/dev/mem",O_RDWR);
  if(fp == -1){
    printf("\n error\n");
    return(-1);               // failed open
  }
  ptr=(unsigned long*)mmap(NULL,getpagesize(),PROT_READ|PROT_WRITE,MAP_SHARED,fp,0x800C0000);
  if(ptr == MAP_FAILED){
    printf("\n Unable to map memory space \n");
    return(-1);
  }
  VIC2SoftInt=ptr+6;
  //point to the address of the register VIC2SoftInt

  if (argc < 2){
    printf("usage: %s port\n", argv[0]);
    exit(0);
  }
  // argv[0] is our source file ID, argv[1] is our port number
  sock=socket(AF_INET,SOCK_DGRAM,0);
  if(sock<0){
    error("ERROR opening socket");
  }
  // create our socket
  char hostname[40];
  char myaddr[10];
  struct hostent *host;
  struct in_addr **boardIPlist;
  gethostname(hostname,sizeof(hostname));
  host = (struct hostent*)gethostbyname(hostname);
  boardIPlist=(struct in_addr**)(host->h_addr_list);
  strcpy(myaddr,inet_ntoa(*boardIPlist[0]));
  // get our local IP address

  port=atoi(argv[1]);
  bzero(&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);
  if(bind(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
    error("binding");
  }
  // bind our socket

  if(setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&boolval,sizeof(boolval))<0){
    printf("error setting socket options\n");
    exit(-1);
  }
  // set socket so that it can broadcast messages

  fromlen=sizeof(struct sockaddr_in);
  int random;

  //obtain random number which means the votes we get
  char s[2];
  char addrrandom[15];
  char buffer[40];
  char buff1[]="WHOIS";
  char buff2[]="VOTE";
  char buf[3];
  buf[2]='\0';
  //the message we need to broadcast @A@B@C@D@E
  int K=0;
  //the lock of whether we are the master
  //if R=1, we are the master, else we are the slave
  pthread_create(&broadcast_thread,NULL,(void*)&broadcast,NULL);
  //thread which is used for reading FIFO from kernel and broadcast that messages
  while(1){
    bzero(buffer,40);
    recvfrom(sock,buffer,40,0,(struct sockaddr*)&anybodyaddr,&fromlen);
    //receive messages from client. it will be blocked if no messages are received
    if((strstr(buffer,buff1)!=NULL)&&(R==1)){
      //not only when we receive WHOIS but also we are the master can we send

      anybodyaddr.sin_addr.s_addr=inet_addr("10.3.52.255");
      bzero(buffer,40);
      strcpy(buffer,"Huang on board ");
      strcat(buffer,myaddr);
      strcat(buffer," is the master");
      //Huang on board 10.3.52.X is the master if I am the master
      sendto(sock,buffer,40,0,(struct sockaddr*)&anybodyaddr,fromlen);
      //send the message above to client
    }
    else if(strstr(buffer,buff2)!=NULL){
      anybodyaddr.sin_addr.s_addr=inet_addr("10.3.52.255");
      srand(time(NULL));
      random=1+rand()%10;
      sprintf(s,"%d",random);
      strcpy(addrrandom,"# ");
      strcat(addrrandom,myaddr);
      strcat(addrrandom," ");
      strcat(addrrandom,s);
      //combine my local ip address to the random vote number
      //the result string will be like # 10.3.52.X Y
      sendto(sock,addrrandom,15,0,(struct sockaddr*)&anybodyaddr,fromlen);
      // send my local ip and random vote numbers to client
      bzero(buffer,40);
      K=1;// to indicate that voting has been executed
      R=1;// if no IP, I am the master
    }
    else if(buffer[0]=='#'){
      if(atoi(&buffer[12])>atoi(&addrrandom[12])){
        R=0;
        K=0;
        //if I am a slave after comparing, I will never be the master unless we vote again
      }
      else if(atoi(&buffer[12])<atoi(&addrrandom[12])){
        if((R==0)&&(K==0)){
          R=0;
        }
        else{
          R=1;
        }
      }
      //compare vote numbers
      else if(atoi(&buffer[12])==atoi(&addrrandom[12])){
        // when tie occurs, compare last bits of ip address
        if(atoi(&buffer[10])>atoi(&addrrandom[10])){
          R=0;
          K=0;
          //if I am a slave after comparing, I will never be the master unless we vote again
        }
        else if(atoi(&buffer[10])<atoi(&addrrandom[10])){
          if((R==0)&&(K==0)){
            R=0;
          }
          else{
            R=1;
          }
        }
      }
    }
    else if((buffer[0]=='@')&&(R==1)&&(buf[1]!=buffer[1])){
      strncpy(buf,buffer,2);                             //copy buffer[0] buffer[1] to buf
      if((fd_fifo_in = open("/dev/rtf/0",O_RDWR))<0){    //open fifo from button event
        printf("open fifo error");
        exit(-1);
      }
      if(write(fd_fifo_in,buffer,sizeof(buffer))<0){
        printf("read fifo error");         //read fifo from button event
        exit(-1);
      }
      *VIC2SoftInt |=0x80000000;
      //enable software interrupt
      anybodyaddr.sin_addr.s_addr=inet_addr("10.3.52.255");
      sendto(sock,buf,sizeof(buf),0,(struct sockaddr*)&anybodyaddr,fromlen);
      //broadcast buf[]
      fflush(stdout);
    }
    else if((buffer[0]=='@')&&(R!=1)){
      if((fd_fifo_in = open("/dev/rtf/0",O_RDWR))<0){    //open fifo from button event
        printf("open fifo error");
        exit(-1);
      }
      if(write(fd_fifo_in,buffer,sizeof(buffer))<0){
        printf("read fifo error");         //read fifo from button event
        exit(-1);
      }
      *VIC2SoftInt |=0x80000000;
      //enable software interrupt
    }
  }
  return 0;
}
