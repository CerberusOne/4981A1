//4981 Assignment 1


#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

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

		while(1){

			userInput = getchar();

			//get user input
			//*charInput = getchar();
			if(userInput == 'E'){
				//printf("%s",input);
				write(pipeInputTranslate[1], input, strlen(input));
				
				memset(input, 0, strlen(input));
				i = 0;
			}
			if(userInput == 'J'){
				system("stty raw echo");
				break;
			}
			else{
				//input = appendChar(userInput, input);
				
				//appendChar(*userInput, input);
				write(pipeInputOutput[1], &userInput, sizeof(char));
				//appendChar(userInput, input);
				input[i] = userInput;
				i++;

				//printf("\nBuffer: %s", input);
			}

			

			
			//memset(input, 0, strlen(input));			
		}
	}

	system("stty sane");
	//kill(0) should kill all the processes
	//^k == 11


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


void translate(int pipeInputTranslate[2], int pipeTranslateOutput[2]){
	//printf("Staring output process\n");

	int nread;
	char buffer[MSGSIZE];

	//close the read descriptor 
	close(pipeInputTranslate[1]);		

	//close the write descriptor
	close(pipeTranslateOutput[0]);	

	while(1){
		switch(nread = read(pipeInputTranslate[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				//printf("tranlate read: pipe empty\n");
				sleep(1);
				break;
			default:
				for(int i = 0; i < strlen(buffer); i++){
					switch(buffer[i]){
						case 'a':
							buffer[i] = 'z';
							break;	
					}
				}

				//output(buffer);
				write(pipeTranslateOutput[1], buffer, strlen(buffer));
				memset(buffer, 0, strlen(buffer));
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
				//printf("output input: pipe empty\n");
				//sleep(1);
				if(strlen(buffer) > 0){
					printf("%c", buffer[0]);
				}
				memset(buffer, 0, strlen(buffer));
				break;
			default:
				break;
		}

		switch(nread = read(pipeTranslateOutput[0], buffer, MSGSIZE)){
			case -1:
			case 0:
				//printf("output translate: pipe empty\n");
				//sleep(1);
				nread = read(pipeTranslateOutput[0], buffer, MSGSIZE);
		
				if(strlen(buffer) > 0){
					printf("%s", buffer);
				}

				memset(buffer, 0, strlen(buffer));
				break;
			default:
				break;
		}

		fflush(stdout);	

		//read and print buffer from pipes

		// nread = read(pipeInputOutput[0], buffer, MSGSIZE);
		// if(strlen(buffer) > 0){
		// 	printf("%s", buffer);
		// }

		// memset(buffer, 0, strlen(buffer));

		// nread = read(pipeTranslateOutput[0], buffer, MSGSIZE);
		
		// if(strlen(buffer) > 0){
		// 	printf("%s\n", buffer);
		// }

		// memset(buffer, 0, strlen(buffer));
	}
}


