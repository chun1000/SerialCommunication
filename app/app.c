#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define CHAT_DEV_PATH_NAME "/dev/chat_dev1"
#define CHAT_MAJOR_NUMBER 502
#define CHAT_MINOR_NUMBER 100
#define CHAT_DEV_NAME "chat_dev"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04 //000 input 001 output
#define GPSET0 0x1c
#define GPLEV0 0x34
#define GPCLR0 0x28


#define IOCTL_MAGIC_NUMBER 'j'
#define CHAT_SET _IOWR(IOCTL_MAGIC_NUMBER, 0, char)
#define CHAT_SEND_CHAR _IOWR(IOCTL_MAGIC_NUMBER, 1, char)
#define CHAT_RECEIVE_CHAR _IOWR(IOCTL_MAGIC_NUMBER, 2, char)
#define CHAT_HAND_SHAKE _IOWR(IOCTL_MAGIC_NUMBER, 3, char)
#define CHAT_EXIT _IOWR(IOCTL_MAGIC_NUMBER, 4, char)

int fd;
char trash = 0xff;
char name[256];

void *send_thread(void *data)
{
   char input;
   char str[32] = {0,};
   char input2 = '\0';
   while(1)
   {
      fgets(str, sizeof(str), stdin);
      
      if(strcmp("EXIT_CHAT_SYSTEM\n", str) == 0)
      {
         ioctl(fd, CHAT_EXIT, &input2);
         exit(0);
      }
      for(int i=0; i<strlen(str);i++)
      {
         input = str[i];      
         ioctl(fd,CHAT_SEND_CHAR,&input);
      }
      ioctl(fd,CHAT_SEND_CHAR,&input2);
      printf("Send Occur!\n");
   }
}

void *receive_thread(void *data)
{
   char input;
   char buffer[32];
   int i = 0;
   while(1)
   {
         ioctl(fd, CHAT_RECEIVE_CHAR, &input);
         if(input == '\0') {
            buffer[i] = '\0';
            printf("The other: %s", buffer);
            i=0;
         }
         else {
               buffer[i] = input;
               i++;
         }
   }
}

int main(void)
{
   dev_t chat_dev;
   
   char input = 0;
   char p;
   int x=0;
   int t_result;
   chat_dev=makedev(CHAT_MAJOR_NUMBER, CHAT_MINOR_NUMBER);
   mknod(CHAT_DEV_PATH_NAME, S_IFCHR|0666, chat_dev);
   fd=open(CHAT_DEV_PATH_NAME, O_RDWR);
   
   
   if(fd<0){
      printf("fail to open chat\n");
      return -1;
   }
 
   printf("Hand Shake Start!\n");
   ioctl(fd,CHAT_SET, &input);
   ioctl(fd, CHAT_HAND_SHAKE, &p);
   sleep(1);
   printf("Chat System Start! Press any Key\n");
   //scanf("%c", &p);
   
   pthread_t p_thread[2];
   int th_id;
   
   th_id = pthread_create(&p_thread[0], NULL, receive_thread, NULL);
   
   if(th_id < 0) {
      printf("thread error\n");
      exit(0);
   }
   
   th_id = pthread_create(&p_thread[1], NULL, send_thread, NULL);
   
   pthread_join(p_thread[0], (void*)&t_result);
   pthread_join(p_thread[1], (void*)&t_result);
   
   close(fd);
   
   return 0;
}
