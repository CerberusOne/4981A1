//4981 Assignment 1


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

int main(){
	char userInput;
	char input[MSGSIZE] = {0};			//buffer used to pass strings to the translate process
	int i = 0;

	int pipeInputOutput[2];			//pipe between Input and Output processes
	int pipeInputTranslate[2];		//pipe between Input and Translate processes
	int pipeTranslateOutput[2];		//pipe between Translate and Output processes

	pid_t childOutputpid;			//process id for output process
	pid_t childTranslatepid;		//process id for Translate process

	system("stty raw -echo");	

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


	childOutputpid = fork();	//create the output fork

	//validate output fork
	if(childOutputpid == -1){
		perror("Fork Failed!\n");
		return 1;
	}

	//check if this is the child process for output
	if(childOutputpid == 0){
		//perror("Child fork, starting Output\n");
		output(pipeInputOutput, pipeTranslateOutput);
	}
	else{
		childTranslatepid = fork();		//create the translate fork

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
			userInput = getchar();
			fflush(stdout);		//flsuh the stdout after any prints

			switch(userInput){
				case 'E':	
					write(pipeInputOutput[1], &userInput, sizeof(char));
					write(pipeInputTranslate[1], input, sizeof(input));
					memset(input, 0, MSGSIZE);
					i = 0;
					break;
				// case 'J':	
				// 	system("stty raw echo");
				// 	break;
				case 11:
					sleep(1);
					kill(0, SIGINT);
					return 0;
				case 'T':
					write(pipeInputOutput[1], &userInput, sizeof(char));
					write(pipeInputTranslate[1], input, sizeof(input));
					sleep(2);

					system("stty sane");
					kill(0, SIGINT);
					return 0;
				default:
					write(pipeInputOutput[1], &userInput, sizeof(char));
					input[i] = userInput;
					i++;
			}

			fflush(stdout);	
		}
	}

	return 0;
}


//erase "X"
//line kill "K"


void translate(int pipeInputTranslate[2], int pipeTranslateOutput[2]){
	int nread;
	char buffer[MSGSIZE];
	char outputBuffer[MSGSIZE];
	int i, j;

	//close the read descriptor 
	close(pipeInputTranslate[1]);		

	//close the write descriptor
	close(pipeTranslateOutput[0]);	

	while(1){
		fflush(stdout);
		switch(nread = read(pipeInputTranslate[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				for(i = 0, j = 0; i < strlen(buffer); i++, j++){
					switch(buffer[i]){
						case 'a':
							outputBuffer[j] = 'z';
							break;	
						case 'X':
							j-=2;
							break;
						case 'K':
							memset(outputBuffer, 0, strlen(outputBuffer));
							outputBuffer[0] = ' ';
							i = strlen(buffer);		//stop the for loop
							j = 1;
							break;
						default:
							outputBuffer[j] = buffer[i];
					}
				}

				if(j == 0){
					memset(outputBuffer, 0, strlen(outputBuffer));
					outputBuffer[0] = ' ';
					i = strlen(buffer);	//stop the for loop
					j = 1;
				}
				
				write(pipeTranslateOutput[1], outputBuffer, j);
				memset(buffer, 0, strlen(buffer));
				memset(outputBuffer, 0, strlen(outputBuffer));

				fflush(stdout);	
		}
	}
}

void output(int pipeInputOutput[2], int pipeTranslateOutput[2]){
	int nread;
	char buffer[MSGSIZE];

	//close the read descriptors 
	close(pipeInputOutput[1]);		
	close(pipeTranslateOutput[1]);	 
	
	while(1){
		fflush(stdout);
		switch(nread = read(pipeInputOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				if(strlen(buffer) > 0){
					printf("%c", buffer[0]);
				}
				memset(buffer, 0, strlen(buffer));
				break;
		}

		fflush(stdout);	
		switch(nread = read(pipeTranslateOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				if(strlen(buffer) > 0){
					sleep(1);
					
					//new line after the last character was echoed
					//another new line after printing the translation
					printf("\n\r%s\n\r", buffer);
				}
				else if(strlen(buffer) == 0){
					printf("\n\r");
				}

				memset(buffer, 0, strlen(buffer));
				break;
		}

		fflush(stdout);	
	}
}


