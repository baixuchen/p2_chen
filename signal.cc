#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>


void print(int y)
{
 printf("you can't kill me :D\n");
}
int main(int argc, char *argv[])
{
 
 struct sigaction act, oact;
 act.sa_handler = print;
 sigemptyset(&act.sa_mask);
 act.sa_flags = 0;
 sigaction(SIGINT, &act, NULL);
 while(1);
 printf("yea2");

}
