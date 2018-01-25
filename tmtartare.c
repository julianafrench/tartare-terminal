/*------------------------------------------------------------------------
-- SOURCE FILE:   tmtartare - A console application that turns off
--                            regular terminal processing and
--                            replaces some of it with a little
--                            pizzazz.
--
-- PROGRAM:     tmtartare
--
-- FUNCTIONS:   void input(int, int, int[]);
--              void output(int);
--              void translate(int, int);
--
-- DATE:        January 18, 2018
--
-- REVISIONS:   January 21 - output function uses child process
                January 24 - translate functionality added
--
-- DESIGNER:    Juliana French
--
-- PROGRAMMER:  Juliana French
--
-- NOTES:
-- This program disables all regular terminal processing in favour of
-- certain edits. It lets a user type on the screen and immediately
-- shows what they are typing on the screen. Once an "enter" key is
-- pressed, the program then "translates" the keyboard input.
-- It also uses separate processes for all major functions, using
-- pipes and signals to communicate between them.
--
-- The program is named "tmtartare" because it puts the keyboard in
-- "raw" mode. Like beef tartare, terminal tartare is raw but with a
-- few extra flavours added to taste :).
------------------------------------------------------------------------*/
#include "tmtartare.h"

/*------------------------------------------------------------------------
-- FUNCTION:     main
--
-- DATE:         Janyary 18, 2018
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:    Juliana French
--
-- PROGRAMMER:  Juliana French
--
-- INTERFACE:   main(void)
--
-- RETURNS:     void
--
-- NOTES:
-- The main entry point into the program, this function turns off
-- regular keyboard processing, creates the necessary processes and pipes,
-- and calls all the other functions to handle user input.
------------------------------------------------------------------------*/
int main(void)
{
  // make process id array and pipe arrays.
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
  // error check first fork
  if (pid[0] < 0)
  {
    perror("output fork failed");
    exit(1);
  }

  if (pid[0] == 0)
  {
    // child is "output" process, read from pipe
    output(out_pipe[0]);
  }
  else
  {
    // in parent, fork again for translate process
    pid[1] = fork();
    if (pid[1] < 0)
    {
      perror("translate fork failed");
      exit(1);
    }

    if (pid[1] == 0)
    {
      // child is "translate" process
      translate(out_pipe[1], trans_pipe[0]);
    }

    // otherwise in parent process, write to both pipes
    input(out_pipe[1], trans_pipe[1], pid);
  }

  // turn on regular processing before exiting
  system("stty -raw -igncr echo");
  exit(0);
}

/*------------------------------------------------------------------------
-- FUNCTION:    input
--
-- DATE:        January 24, 2018
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:    Juliana French
--
-- PROGRAMMER:  Juliana French
--
-- INTERFACE:   void input(int pout, int ptrans, int children[])
--                  int pout : the output pipe descriptor
--                  int ptrans : the translate pipe descriptor
--                  int children[] : the children process IDs
--
-- RETURNS:     void
--
-- NOTES:
-- This function reads user input character by character, checking for
-- special characters and writing to the necessary pipes to hand the
-- data off to the other processes running.
------------------------------------------------------------------------*/
void input(int pout, int ptrans, int children[])
{
  char inbuffer[PIPEBUFSIZE];
  size_t transIndex = 0;

  while (1)
  {
    char c = getchar();
    switch(c)
    {
      case 11:
        kill(children[0], SIGTERM);
        kill(children[1], SIGTERM);
        return;
      case 'T':
        inbuffer[transIndex++] = c;
        write(pout, &c, 1);
        write(ptrans, inbuffer, transIndex);
        kill(children[0], SIGTERM);
        kill(children[1], SIGTERM);
        return;
      case 'E':
        inbuffer[transIndex++] = c;
        // write to both, reset buffer
        write(pout, &c, 1);
        write(pout, "\n\r", 2);
        write(ptrans, inbuffer, transIndex);
        transIndex = 0;
        break;
      case 'K':
        write(pout, &c, 1);
        transIndex = 0;
        inbuffer[transIndex] = '\0';
        break;
      default:
        // store in buffer
        inbuffer[transIndex++] = c;
        write(pout, &c, 1);
        break;
    }
  }
}

/*------------------------------------------------------------------------
-- FUNCTION:    output
--
-- DATE:        January 18, 2018
--
-- REVISIONS:   January 24 - overhauled to check for termination
--              character only and flush stdout.
--
-- DESIGNER:    Juliana French
--
-- PROGRAMMER:  Juliana French
--
-- INTERFACE:   void output(int pout)
--                  int pout : the output pipe file descriptor
--
-- RETURNS:     void
--
-- NOTES:
-- Checks for terminating character 'T' and flushes stdout before exiting
-- the program.
------------------------------------------------------------------------*/
void output(int pout)
{
  char out_char;
  int exit = 0;
  while(1)
  {
    if (read(pout, &out_char, 1) == 1)
    {
      if (out_char == 'T')
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
      fprintf(stdout, "%c", out_char);
      fflush(stdout);
    }
  }
  fprintf(stdout, "%s", "\n\r");
  fflush(stdout);
}

/*------------------------------------------------------------------------
-- FUNCTION:    translate
--
-- DATE:        January 24, 2018
--
-- REVISIONS:   (Date and Description)
--
-- DESIGNER:    Juliana French
--
-- PROGRAMMER:  Juliana French
--
-- INTERFACE:   void translate(int pout, int ptrans)
--                  int pout : the output pipe file descriptor
--                  int ptrans : the translate pipe file descriptor
--
-- RETURNS:     void
--
-- NOTES:
-- This function "translates" characters from output and relays it back:
-- X functions as erasing a character
-- E brings the cursor to a new line
-- T deletes its character like X but also terminates the program
-- a gets turned into z.
------------------------------------------------------------------------*/
void translate(int pout, int ptrans)
{
  char c_read;

  while (1)
  {
    if (read(ptrans, &c_read, 1) == 1)
    {
      // look at pipe buffer
      switch (c_read)
      {
        case 'X':
          write(pout, "\b \b", 3);
          break;
        case 'E':
          write(pout, "\n\r", 2);
          break;
         case 'T':
         // don't print T if line is not empty
          write(pout, "\b", 1);
          break;
        case 'a':
          // change a to z then fall through to default
          c_read = 'z';
        default:
          write(pout, &c_read, 1);
      }
    }
  }
}
