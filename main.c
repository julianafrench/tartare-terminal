#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define PIPERW 2
#define PIPEBUFSIZE 128

void input(int pout, int ptrans);
void output(int pin);
void translate(int pout, int pin);

int main(int argc, char* argv[])
{
  int pid[2] = {1, 1};
  int out_pipe[PIPERW], trans_pipe[PIPERW];

  system("stty raw igncr -echo");



  // open output and translate pipes
  if (pipe(out_pipe) < 0 || (pipe(trans_pipe) < 0))
  {
    perror("pipe call: error ");
    exit(1);
  }

  pid[0] = fork();
  // error check fork
  if (pid[0] < 0)
  {
    perror("output fork failed : error ");
    exit(1);
  }

  if (pid[0] == 0)
  {
    // child is "output" process, read from pipe
    fprintf(stderr, "output pid: %d\n\r", getpid());
    setpgid(0, 0);
    output(out_pipe[0]);
  }
  else
  {
    // in parent, fork again for translate process
    pid[1] = fork();
    if (pid[1] < 0)
    {
      perror("translate fork failed: error");
      exit(1);
    }

    if (pid[1] == 0)
    {
      // child is "translate" process
      fprintf(stderr, "translate pid:%d\n\r", getpid());
      setpgid(0, 0);
      translate(out_pipe[1], trans_pipe[0]);
    }

    // otherwise in parent process, write to both pipes
    fprintf(stderr, "parent pid: %d\n\r", getpid());
    input(out_pipe[1], trans_pipe[1]);
  }
  system("stty -raw -igncr echo");
  kill(-pid[0], SIGTERM);
  exit(0);
}

void input(int pout, int ptrans)
{
  char inbuffer[PIPEBUFSIZE];
  size_t transIndex = 0;

  while (1)
  {
    char c = getchar();
    switch(c)
    {
      case 11:
        kill(0, SIGKILL);
        break;
      case 'T':
        inbuffer[transIndex++] = c;
        write(pout, &c, 1);
        write(ptrans, inbuffer, transIndex);
        return;
      case 'E':
        inbuffer[transIndex++] = c;
        // write to both, reset buffer
        write(pout, &c, 1);
        write(ptrans, inbuffer, transIndex);
        transIndex = 0;
        break;
      default:
        // store in buffer
        inbuffer[transIndex++] = c;
        write(pout, &c, 1);
        break;
    }
  }
  //fprintf(stderr, "%s", inbuffer);
}

void output(int pout)
{
  char outbuffer[1];
  int exit = 0;
  while(1)
  {
    if (read(pout, outbuffer, 1) == 1)
    {
      if (outbuffer[0] == 'T')
      {
        if (exit == 1)
        {
          break;
        }
        else
        {
          exit = 1;
        }
      }
      fprintf(stdout, "%c", outbuffer[0]);
      fflush(stdout);
    }
  }
  fprintf(stdout, "%s", "\n\r");
  fflush(stdout);
}

void translate(int pout, int ptrans)
{
  char c_read;

  while (1)
  {
    if (read(ptrans, &c_read, 1) == 1)
    {
      switch (c_read)
      {
        case 'X':
          write(pout, "\b \b", 3);
          break;
        case 'E':
          write(pout, "\n\r", 2);
          break;
        case 'T':
          write(pout, &c_read, 1);
          return;
        case 'a':
          // change a to z then fall through to default
          c_read = 'z';
        default:
          write(pout, &c_read, 1);
      }
    }
  }
}
