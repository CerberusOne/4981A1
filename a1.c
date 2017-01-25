//4981 Assignment 1


#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define MSGSIZE 16

void output(int pipeInputOutput[2], int pipeTranslateOutput[2]);
void translate(int pipeInputOutput[2], int pipeTranslateOutput[2]);
void appendChar(char userInput, char* buffer);

int main(){
	char userInput;
	char input[256];			//buffer used to pass strings to the translate process
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


		//add every keystroke (getch?) to buffer
		//echo using Output(getch)
		//if ENTER: translate(buffer), clear buffer
		//if control-k, exit()	
		fflush(stdout);	

		while(1){

			userInput = getchar();

			switch(userInput){
				case 'E':
					write(pipeInputTranslate[1], input, strlen(input));
					memset(input, 0, strlen(input));
					i = 0;
					break;
				case 'J':	
					system("stty raw echo");
					break;
				case 11:
					exit(1);
				case 'T':
					system("stty sane");
					kill(childOutputpid, SIGINT);
					kill(childOutputpid, SIGINT);
					exit(1);
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


void appendChar(char userInput, char* buffer){
	//printf("%s", userInput);


	int len = sizeof(buffer);
	char *str = malloc(len +2);
	strcpy(str, buffer);
	str[len] = userInput;
	str[len+1] = '\0';

	buffer = str;
}

//erase "X"
//line kill "K"


void translate(int pipeInputTranslate[2], int pipeTranslateOutput[2]){
	int nread;
	char buffer[MSGSIZE];
	char outputBuffer[MSGSIZE];

	//close the read descriptor 
	close(pipeInputTranslate[1]);		

	//close the write descriptor
	close(pipeTranslateOutput[0]);	

	while(1){
		switch(nread = read(pipeInputTranslate[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:


				for(int i = 0; i < strlen(buffer); i++){
					switch(buffer[i]){
						case 'a':
							outputBuffer[i] = 'z';
							break;	
						case 'X':
						default:
							outputBuffer[i] = buffer[i];

					}
				}

				write(pipeTranslateOutput[1], outputBuffer, strlen(buffer));
				memset(buffer, 0, strlen(buffer));
				memset(buffer, 0, strlen(outputBuffer));
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

		switch(nread = read(pipeTranslateOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				break;
			default:
				if(strlen(buffer) > 0){
					

					printf("\n\r%s\n\r", buffer);
				}

				memset(buffer, 0, strlen(buffer));
				break;
		}

		fflush(stdout);	
	}
}


