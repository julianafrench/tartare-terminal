#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 2

int input(int p, char* buf);
int output(int p, char* buf);
int control();

int main(int argc, char* argv[])
{
  system("stty raw igncr -echo");
  int pid[2] = {1, 1};
  int pipes[2];
  char inbuffer[BUFSIZE];


  // create process and open pipe
  if (pipe(pipes) < 0)
  {
    perror("pipe call: error 1");
    exit(1);
  }

  pid[0] = fork();
  // error check fork
  if (pid[0] < 0) {
    perror("fork failed : error 2");
    exit(2);
  }

  if (pid[0] == 0)
  {
    // read from pipe
    output(pipes[0], inbuffer);
  }
  else
  {
    // write to pipe
    input(pipes[1], inbuffer);

  }
  system("stty -raw -igncr echo");
  exit(0);
}

int input(int p, char* buf)
{
  while (fgets(buf, BUFSIZE, stdin))
  {
      write(p, buf, BUFSIZE);
  }
  return 1;
}

int output(int p, char* buf)
{
  read(p, buf, BUFSIZE);
  fprintf(stdout, "%s", buf);
  return 1;
}
