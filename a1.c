/*-----------------------------------------------------------------------
--  SOURCE FILE:   a1.cpp   An application that utilizes forks, pipes, and
--							signals to read characters from a user and translates
--							them accordingly. 
--
--  PROGRAM:       COMP 4981 Assignment 1
--
--  FUNCTIONS:		void output(int pipeInputOutput[2], int pipeTranslateOutput[2]);
--					void translate(int pipeInputOutput[2], int pipeTranslateOutput[2]);
--					void appendChar(char userInput, char* buffer);
--                  
--
--  DATE:          	Jan 21, 2017
--
--  DESIGNER:      Aing Ragunathan
--
--
--  NOTES:
--		This application utilizes forking to complete tasks with multiple
--		multiples processes. It also communicates between them using pipes and
--		signals.
--		This application also turns off echo in the Linux terminal and resets it
--		upon termination, whether forced or not.
--		
--		Users can make use of commands to control the translator:
--			'X' 		- erase last character
--			'K' 		- kill the entire line
--			'control-k'	- (interrupt) abnormal termination of the program
--			'T'			- normal termination of the program
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define MSGSIZE 256

void output(int pipeInputOutput[2], int pipeTranslateOutput[2]);
void translate(int pipeInputOutput[2], int pipeTranslateOutput[2]);
void appendChar(char userInput, char* buffer);

/*---------------------------------------------------------------------------------
--  FUNCTION:      main
--
--  DATE:          Jan 21, 2017
--
--  DESIGNER:      Aing Ragunathan
--
--
--  INTERFACE:     main()
--                        
--
--  RETURNS:      int
--
--  NOTES:
--		This function sets up processes for inputting, translating and outputing 
--		characters from the terminal back to the terminal. It also creates pipes 
--		in order to send information between the 3 processes. 
--		
--		This main function takes input, sends it to the output process to be echo'ed
--		back to the user. It also sends a buffer to the translate process to handle
--		for certain characters.
-----------------------------------------------------------------------------------*/

int main(){
	char userInput;					//buffer for character entered by the user
	char input[MSGSIZE] = {0};		//buffer used to hold strings before passing 
										//to translate process
	int i = 0;						//counter, keeps track of string buffer size

	int pipeInputOutput[2];			//pipe between Input and Output processes
	int pipeInputTranslate[2];		//pipe between Input and Translate processes
	int pipeTranslateOutput[2];		//pipe between Translate and Output processes

	pid_t childOutputpid;			//process id for output process
	pid_t childTranslatepid;		//process id for Translate process

	system("stty raw -echo");		//turns Linux terminal echo off

	//Validate that all the pipe arrays were created.
	if(pipe(pipeInputOutput) < 0){
		perror("pipeInputOutput failed\n");
		exit(1);
	}
	if(pipe(pipeInputTranslate) < 0){
		perror("pipeInputTranslate failed\n");
		exit(1);
	}
	if(pipe(pipeTranslateOutput) < 0){
		perror("pipeTranslateOutput failed\n");
		exit(1);
	}



	//Open all the pipes
	if(fcntl(pipeInputOutput[0], F_SETFL, O_NDELAY) < 0){
		perror("fcntl call\n");
		exit(1);
	}
	if(fcntl(pipeInputTranslate[0], F_SETFL, O_NDELAY) < 0){
		perror("fcntl call\n");
		exit(1);
	}
	if(fcntl(pipeTranslateOutput[0], F_SETFL, O_NDELAY) < 0){
		perror("fcntl call\n");
		exit(1);
	}

	//create the output fork
	childOutputpid = fork();	

	//validate output fork
	if(childOutputpid == -1){
		perror("Fork Failed!\n");
		return 1;
	}

	//check if this is the child process for output
	if(childOutputpid == 0){
		output(pipeInputOutput, pipeTranslateOutput);
	}
	else{
		//create the translate fork
		childTranslatepid = fork();		

		//validate translate fork
		if(childTranslatepid == -1){
			//perror("Translate fork failed\n");
			return 1;
		}

		//check if this is the child process for translate
		if(childTranslatepid == 0){
			//perror("Child fork, starting Translate\n");
			translate(pipeInputTranslate, pipeTranslateOutput);
		}
		
		//close the write descriptor
		close(pipeInputOutput[0]);		
		close(pipeTranslateOutput[0]);	

		//flush the stdout before starting
		fflush(stdout);	

		while(1){
			fflush(stdin);
			userInput = getchar();		//get an input character from the user
			fflush(stdout);				//flush the stdout after any prints

			switch(userInput){
				case 'E':				//'E' is equivalent to the ENTER key
					//send the input character the output process
					write(pipeInputOutput[1], &userInput, sizeof(char));
					//send the input buffer to the translate process
					write(pipeInputTranslate[1], input, sizeof(input));
					//clear the input buffer
					memset(input, 0, MSGSIZE);
					//reset the buffer position counter
					i = 0;
					break;
				case 11:				//abnormal termination, doesn't translate
					sleep(1);
					kill(0, SIGINT);
					return 0;
				case 'T':				//normal termination, prints the transation
					//send the character to the output process
					write(pipeInputOutput[1], &userInput, sizeof(char));
					//send the input buffer to the translate process
					write(pipeInputTranslate[1], input, sizeof(input));
					//wait for translate and output processes to finsh printing before 
						//safelty terminating the program  
					sleep(2);
					//reset the Linux terminal settings before terminatin the program
					system("stty sane");
					//kill all processes associated with this process
					kill(0, SIGINT);
					return 0;
				default:				//adds the character to the buffer string
					//send the character to the output process
					write(pipeInputOutput[1], &userInput, sizeof(char));
					//append the character to the buffer
					input[i] = userInput;
					//increment the buffer position counter
					i++;
			}

			fflush(stdout);			//flushes the stdout after any writes from output
		}
	}

	return 0;
}


/*---------------------------------------------------------------------------------
--  FUNCTION:      translate
--
--  DATE:          Jan 21, 2017
--
--  DESIGNER:      Aing Ragunathan
--
--
--  INTERFACE:     void translate(int pipeInputTranslate[2], int pipeTranslateOutput[2])
--                        
--
--  RETURNS:      void
--
--  NOTES:
--		This function is created as a process. It takes input from a pipe from the 
--		input process/function and translates it so every 'a' is changed to a 'z' and
--		every 'X' is treated like a backspace (deletes the character before it, as 
--		well as itself).
--		Any 'K's found will triggers a kill line.
--
--		After all translations are done, the translated buffer is sent to the output
--		process/function to get printed to stdout.
-----------------------------------------------------------------------------------*/
void translate(int pipeInputTranslate[2], int pipeTranslateOutput[2]){
	int nread;						//keeps track of any data coming from the pipe 
	char buffer[MSGSIZE];			//buffer to hold incoming data from input
	char outputBuffer[MSGSIZE];		//holds any translated data before sending to output.
	int i, j;						//counters keeping track of buffer positions

	//close the read descriptor 
	close(pipeInputTranslate[1]);	

	//close the write descriptor
	close(pipeTranslateOutput[0]);	

	while(1){
		fflush(stdout);		//flushes the stdout after any writes from output
		switch(nread = read(pipeInputTranslate[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				//loop through every position in the received buffer
				for(i = 0, j = 0; i < strlen(buffer); i++, j++){
					switch(buffer[i]){
						case 'a':		//change any 'a' characters to 'z'
							outputBuffer[j] = 'z';
							break;	
						case 'X':		//treat any 'X' characters like backspaces
							if(i == 0){
								j--;
							}
							else{
								j-=2;
							}
							break;
						case 'K':		//send blank line to output
							memset(outputBuffer, 0, strlen(outputBuffer));
							outputBuffer[0] = ' ';
							i = strlen(buffer);		//stop the for loop
							j = 1;
							break;
						default:
							outputBuffer[j] = buffer[i];
					}
				}

				//if all the characters were erased by 'X', send an empty line to output
				if(j == 0){
					memset(outputBuffer, 0, strlen(outputBuffer));
					outputBuffer[0] = ' ';
					j = 1;
				}
				
				sleep(1);
				//send translated buffer to the output process
				write(pipeTranslateOutput[1], outputBuffer, j);
				memset(buffer, 0, strlen(buffer));
				memset(outputBuffer, 0, strlen(outputBuffer));

				fflush(stdout);	//flushes the stdout after any writes from output
		}
	}
}

/*---------------------------------------------------------------------------------
--  FUNCTION:      output
--
--  DATE:          Jan 21, 2017
--
--  DESIGNER:      Aing Ragunathan
--
--
--  INTERFACE:     void output(int pipeInputOutput[2], int pipeTranslateOutput[2])
--                        
--
--  RETURNS:      void
--
--  NOTES:
--		This function is created as a process. It is used to echo characters that the
--		the user inputs. 
--		It also prints translated buffers from translate process to stdout.
-----------------------------------------------------------------------------------*/
void output(int pipeInputOutput[2], int pipeTranslateOutput[2]){
	int nread;					//used to check if there is ouput from a pipe
	char buffer[MSGSIZE];		//holds the buffer coming in from translate or output

	//close the read descriptors 
	close(pipeInputOutput[1]);		//pipe from input to output	
	close(pipeTranslateOutput[1]);	//pipe from translate to output
	
	while(1){
		fflush(stdout);				//flushes the stdout after any writes from output
		//read from the input pipe
		switch(nread = read(pipeInputOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				//echo input back to stdout
				if(strlen(buffer) > 0){
					printf("%c", buffer[0]);
				}
				//reset buffer
				memset(buffer, 0, strlen(buffer));
				break;
		}

		fflush(stdout);				//flushes the stdout after any writes from output
		//read from the translate pipe
		switch(nread = read(pipeTranslateOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				//check if there if data has been pasted through the buffer
				if(strlen(buffer) > 0){					
					//new line after the last character was echoed
						//another new line after printing the translation
					printf("\n\r%s\n\r", buffer);
				}
				else if(strlen(buffer) == 0){
					//prints a new line if buffer was empty (from 'K' or 'X')
					printf("\n\r");
				}
				//reset the buffer
				memset(buffer, 0, strlen(buffer));
				break;
		}

		fflush(stdout);	//flushes the stdout after any writes from output
	}
}


