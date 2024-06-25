#include <iostream>
#include <cstring>
#include <vector>
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

using namespace std;

void execute_subshell(char str[]);

// Function to execute a single command
void execute_command(command *cmd) {
    pid_t pid;
    int status;
	//cout << cmd->args[0];
	//cout << *(cmd->args);
	//cout << cmd->args[1];
    // Fork a child process
    pid = fork();

    if (pid < 0) {
        // Error occurred
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        // Execute the command
        if (execvp(cmd->args[0], cmd->args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        // Wait for the child to complete
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        // Check if child process terminated normally
        if (WIFEXITED(status)) {
            //printf("Child process exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("Child process terminated abnormally\n");
        }
    }
}


void execute_pipeline(parsed_input *input) {
	//cout<<"buraya girdi";
    int pipes[input->num_inputs - 1][2]; // Array of pipes
    pid_t pid;
    int status;
    bool b=false;

	//for(int a=0; a<input->num_inputs ; a++){
	//	if((input->inputs[a]).type==INPUT_TYPE_SUBSHELL) b=true;
	//}
	//if(b) execute_pipeline_sub(input);
    // Create pipes
    for (int i = 0; i < input->num_inputs - 1; ++i) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe not created.");
            exit(EXIT_FAILURE);
        }
    }

    // Loop through each command in the pipeline
    for (int i = 0; i < input->num_inputs; i++) {
        // Fork a child process
        
        
        pid = fork();

        if (pid < 0) {
            // Error occurred
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            // Redirect input from previous command's output
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output to next command's input
            if (i < input->num_inputs - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe fds
            for (int j = 0; j < input->num_inputs - 1; ++j) {
                if (close(pipes[j][0]) == -1 || close(pipes[j][1]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            // Execute the command
            if (input->inputs[i].type == INPUT_TYPE_COMMAND) {
                execvp(input->inputs[i].data.cmd.args[0], input->inputs[i].data.cmd.args);
            } else if (input->inputs[i].type == INPUT_TYPE_SUBSHELL) {
            	//cout<<"subshell çağırıyorum input: "<<i<<endl;
                execute_subshell(input->inputs[i].data.subshell);
            }
        }
    }

    // Close all pipe fds in the parent
    for (int i = 0; i < input->num_inputs - 1; ++i) {
        if (close(pipes[i][0]) == -1 || close(pipes[i][1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < input->num_inputs; ++i) {
        wait(&status);
    }
}





void execute_sequential(parsed_input *input) {
    pid_t pid;
    int status;
    //cout<< input->num_inputs ;
	/*for (int k = 0; k < input->num_inputs; k++) {
		cout<<"buraya girdi2";
		if(input->inputs[k].type == INPUT_TYPE_PIPELINE){
			cout<< "Pipee" ;
		}else if(input->inputs[k].type == INPUT_TYPE_COMMAND){
			cout<< "command" ;
		}
		
	}*/
    // Loop through each command in the input
    for (int i = 0; i < input->num_inputs; i++) {
        // Fork a child process
        
        pid = fork();

        if (pid < 0) {
            // Error occurred
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            // Execute the command
            if (input->inputs[i].type == INPUT_TYPE_PIPELINE) {

				pipeline *pline=(&input->inputs[i].data.pline);
				
				int pipes[pline->num_commands - 1][2]; // Array of pipes
    pid_t pid2;
    int status;

    // Create pipes
    for (int i = 0; i < pline->num_commands - 1; ++i) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe not created");
            exit(EXIT_FAILURE);
        }
    }

    // Loop through each command in the pipeline
    for (int i = 0; i < pline->num_commands; i++) {
        // Fork a child process
        pid2 = fork();

        if (pid2 < 0) {
            // Error occurred
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Child process
            // Redirect input from previous command's output
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output to next command's input
            if (i < pline->num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe fds
            for (int j = 0; j < pline->num_commands - 1; ++j) {
                if (close(pipes[j][0]) == -1 || close(pipes[j][1]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            // Execute the command
            if (execvp(pline->commands[i].args[0], pline->commands[i].args) == -1) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe fds in the parent
    for (int i = 0; i < pline->num_commands - 1; ++i) {
        if (close(pipes[i][0]) == -1 || close(pipes[i][1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < pline->num_commands; ++i) {
        wait(&status);
    }
				
        	}
            else {execute_command(&input->inputs[i].data.cmd);}
            exit(EXIT_SUCCESS); // Exit the child process after executing the command
        } else {
            // Parent process
            // Wait for the child to complete
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            // Check if child process terminated normally
            if (!WIFEXITED(status)) {
                printf("Child process terminated abnormally\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    
}

void execute_parallel(parsed_input *input) {
    pid_t pid;
    int status;

    // Loop through each command or pipeline in parallel
    for (int i = 0; i < input->num_inputs; i++) {
        // Fork a child process
        pid = fork();

        if (pid < 0) {
            // Error occurred
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            if (input->inputs[i].type == INPUT_TYPE_COMMAND) {
                // Execute command
                execute_command(&input->inputs[i].data.cmd);
                exit(EXIT_SUCCESS); // Exit child process after executing command
            } else if (input->inputs[i].type == INPUT_TYPE_PIPELINE) {
                // Execute pipeline
                //exit(EXIT_SUCCESS); // Exit child process after executing pipeline
                pipeline *pline=(&input->inputs[i].data.pline);
				
				int pipes[pline->num_commands - 1][2]; // Array of pipes
    pid_t pid2;
    int status;

    // Create pipes
    for (int i = 0; i < pline->num_commands - 1; ++i) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe not created");
            exit(EXIT_FAILURE);
        }
    }

    // Loop through each command in the pipeline
    for (int i = 0; i < pline->num_commands; i++) {
        // Fork a child process
        pid2 = fork();

        if (pid2 < 0) {
            // Error occurred
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Child process
            // Redirect input from previous command's output
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output to next command's input
            if (i < pline->num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe fds
            for (int j = 0; j < pline->num_commands - 1; ++j) {
                if (close(pipes[j][0]) == -1 || close(pipes[j][1]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            // Execute the command
            if (execvp(pline->commands[i].args[0], pline->commands[i].args) == -1) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe fds in the parent
    for (int i = 0; i < pline->num_commands - 1; ++i) {
        if (close(pipes[i][0]) == -1 || close(pipes[i][1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < pline->num_commands; ++i) {
        wait(&status);
    }
            } else {
                // Handle other input types if necessary
                printf("Unsupported input type\n");
                exit(EXIT_FAILURE); // Exit child process with failure
            }
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < input->num_inputs; ++i) {
        wait(&status);
    }
}

char** splitString(char input[], const char* x) {
    char** tokens = new char*[100]; // Array to store pointers to tokens
    int num_tokens = 0;

    // Tokenize the input string based on spaces
    char* token = std::strtok(input, x );
    while (token != nullptr && num_tokens < 100) {
        // Check if token starts and ends with double quotes
        if (token[0] == '"' && token[strlen(token) - 1] == '"') {
            // If token starts and ends with double quotes, discard them
            token++;
            token[strlen(token) - 1] = '\0';
        }

        tokens[num_tokens] = new char[strlen(token) + 1]; // Allocate memory for the token
        strcpy(tokens[num_tokens], token); // Copy token to dynamically allocated memory
        ++num_tokens;
        token = std::strtok(nullptr, x);
    }

    // Mark the end of the tokens array with a nullptr
    tokens[num_tokens] = nullptr;

    return tokens;
}

// Function to free memory allocated for tokens
void freeTokens(char** tokens) {
    for (int i = 0; tokens[i] != nullptr; ++i) {
        delete[] tokens[i]; // Free memory for each token
    }
    delete[] tokens; // Free memory for the tokens array itself
}

void main2(parsed_input* input){
	//pretty_print(input) ;
	switch (input->separator) {
            case SEPARATOR_NONE:
                // Execute single command or subshell
                if (input->num_inputs == 1) {
                    switch (input->inputs[0].type) {
                        case INPUT_TYPE_COMMAND:
                            execute_command(&input->inputs[0].data.cmd);
                            break;
                        
                        default:
                            printf("Invalid input.\n");
                            break;
                    }
                } else {
                    printf("Invalid input.\n");
                }
                break;
            case SEPARATOR_PIPE:
            	//printf("pipe input.\n");
				execute_pipeline(input);
                break;
            case SEPARATOR_SEQ:
	            //printf("seq input.\n");
                execute_sequential(input);
                
                //if(input.separator == SEPARATOR_PIPE) printf("pipe");                
                //if(input.separator == SEPARATOR_SEQ) printf("seq");
                //if(input.separator == SEPARATOR_PARA) printf("para");  
                break;
            case SEPARATOR_PARA:
                execute_parallel(input);
                //printf("paralel input.\n");
                //if(input.separator == SEPARATOR_PIPE) printf("pipe");                
                //if(input.separator == SEPARATOR_SEQ) printf("seq");
                //if(input.separator == SEPARATOR_PARA) printf("para");  
                break;
            default:
                printf("Invalid input.\n");
                break;
        }

        free_parsed_input(input);


}

void execute_subshell(char str[]){
	//cout<< str[0];
	int commaCount = 0, semicolonCount = 0, pipeCount = 0;

// Iterate through the string
	for (int i = 0; str[i] != '\0'; ++i) {
		// Increment the respective count based on the character encountered
		switch (str[i]) {
		    case ',':
		        commaCount++;
		        break;
		    case ';':
		        semicolonCount++;
		        break;
		    case '|':
		        pipeCount++;
		        break;
		    default:
		        // Do nothing for other characters
		        break;
		}
	}
    //std::cout << "Number of commas: " << commaCount << std::endl;
    //std::cout << "Number of semicolons: " << semicolonCount << std::endl;
    //std::cout << "Number of pipes: " << pipeCount << std::endl;
	if(commaCount == 0 && semicolonCount == 0 && pipeCount == 0){
		parsed_input cmd_input;
		single_input sing;
		single_input_union uni;
		command cmd;
		cmd_input.num_inputs =1;
		cmd_input.separator=SEPARATOR_NONE ;
		sing.type=INPUT_TYPE_COMMAND ;
		//char *args[MAX_ARGS];
		char **args = splitString(str, " ");
		int i;
		for (i = 0; args[i] != nullptr; ++i) {
        	cmd.args[i] = args[i];
    	}
    	cmd.args[i] = nullptr; 
		uni.cmd=cmd;
		sing.data=uni;
		cmd_input.inputs[0] = sing;
		execute_command(&cmd);
		
		
		
		freeTokens(args);
	}else if(commaCount == 0 && semicolonCount == 0 && pipeCount != 0){
		parsed_input cmd_input;
		cmd_input.num_inputs = pipeCount + 1;
		cmd_input.separator = SEPARATOR_PIPE;
		char** inputlar = splitString(str, "|");
		for (int a = 0; a < pipeCount + 1; ++a) {
			single_input sing;
			sing.type = INPUT_TYPE_COMMAND;

			char **args = splitString(inputlar[a], " ");
			command cmd;
			int i;
			for (i = 0; args[i] != nullptr && i < MAX_ARGS - 1; ++i) {
				cmd.args[i] = args[i];
			}
			cmd.args[i] = nullptr; // Ensure the last element is nullptr

			// Copy command into union and single_input
			single_input_union uni;
			uni.cmd = cmd;
			sing.data = uni;
			cmd_input.inputs[a] = sing;

			// Free memory allocated for arguments
			//freeTokens(args);
		}
		//cout<< cmd_input.num_inputs<<endl;
		//cout<< cmd_input.separator<<endl;
		//cout<<cmd_input.inputs[]
		main2(&cmd_input);
		freeTokens(inputlar);
		
	}else if(commaCount != 0 && semicolonCount == 0 && pipeCount == 0){
		parsed_input cmd_input;
		cmd_input.num_inputs = commaCount + 1;
		cmd_input.separator = SEPARATOR_PARA;
		char** inputlar = splitString(str, ",");
		for (int a = 0; a < commaCount + 1; ++a) {
			single_input sing;
			sing.type = INPUT_TYPE_COMMAND;

			char **args = splitString(inputlar[a], " ");
			command cmd;
			int i;
			for (i = 0; args[i] != nullptr && i < MAX_ARGS - 1; ++i) {
				cmd.args[i] = args[i];
			}
			cmd.args[i] = nullptr; // Ensure the last element is nullptr

			// Copy command into union and single_input
			single_input_union uni;
			uni.cmd = cmd;
			sing.data = uni;
			cmd_input.inputs[a] = sing;

			// Free memory allocated for arguments
			//freeTokens(args);
		}
		main2(&cmd_input);
		freeTokens(inputlar);
	
	}else if(commaCount == 0 && semicolonCount != 0 && pipeCount == 0){
		parsed_input cmd_input;
		cmd_input.num_inputs = semicolonCount + 1;
		cmd_input.separator = SEPARATOR_SEQ;
		char** inputlar = splitString(str, ";");
		for (int a = 0; a < semicolonCount + 1; ++a) {
			single_input sing;
			sing.type = INPUT_TYPE_COMMAND;

			char **args = splitString(inputlar[a], " ");
			command cmd;
			int i;
			for (i = 0; args[i] != nullptr && i < MAX_ARGS - 1; ++i) {
				cmd.args[i] = args[i];
			}
			cmd.args[i] = nullptr; // Ensure the last element is nullptr

			// Copy command into union and single_input
			single_input_union uni;
			uni.cmd = cmd;
			sing.data = uni;
			cmd_input.inputs[a] = sing;

			// Free memory allocated for arguments
			//freeTokens(args);
		}
		main2(&cmd_input);
		freeTokens(inputlar);
	
	}else if(commaCount == 0 && semicolonCount != 0 && pipeCount != 0){
		parsed_input cmd_input;
		cmd_input.num_inputs = semicolonCount + 1;
		cmd_input.separator = SEPARATOR_SEQ;
		char** inputlar = splitString(str, ";");
		for (int a = 0; a < semicolonCount + 1; ++a) {
			single_input sing;
			int pipeCount2=0;
			for (int ii = 0; inputlar[a][ii] != '\0'; ++ii) {
				if (inputlar[a][ii] == '|') {
				    pipeCount2++;
				}
			}
			if(pipeCount2==0){		
				sing.type = INPUT_TYPE_COMMAND;
				char **args = splitString(inputlar[a], " ");
				command cmd;
				int i;
				for (i = 0; args[i] != nullptr && i < MAX_ARGS - 1; ++i) {
					cmd.args[i] = args[i];
				}
				cmd.args[i] = nullptr; // Ensure the last element is nullptr

				// Copy command into union and single_input
				single_input_union uni;
				uni.cmd = cmd;
				sing.data = uni;
				cmd_input.inputs[a] = sing;
			}
			else{
				sing.type = INPUT_TYPE_PIPELINE;
				char** inputlar2 = splitString(inputlar[a], "|");
				int num_com= pipeCount2 + 1;
				single_input_union uni;
				pipeline plineobj;
				plineobj.num_commands=num_com;
				//implement creating commands
				for (int i = 0; i < num_com; ++i) {
					// Split the command into arguments
					char** args = splitString(inputlar2[i], " ");
					
					// Store the arguments in the command structure
					command cmd;
					int j;
					for (j = 0; args[j] != nullptr; ++j) {
						cmd.args[j] = args[j];
					}
					cmd.args[j] = nullptr; // Mark the end of arguments
					
					// Store the command in the pipeline
					plineobj.commands[i] = cmd;
					
					// Free memory allocated for arguments
					//freeTokens(args);
				}
				uni.pline=plineobj;
				sing.data=uni;
				cmd_input.inputs[a]=sing;
				freeTokens(inputlar2);
				
			}
		}
		main2(&cmd_input);
		freeTokens(inputlar);
	
	}else if(commaCount != 0 && semicolonCount == 0 && pipeCount != 0){
		parsed_input cmd_input;
		cmd_input.num_inputs = commaCount + 1;
		cmd_input.separator = SEPARATOR_PARA;
		char** inputlar = splitString(str, ",");
		for (int a = 0; a < commaCount + 1; ++a) {
			single_input sing;
			int pipeCount2=0;
			for (int ii = 0; inputlar[a][ii] != '\0'; ++ii) {
				if (inputlar[a][ii] == '|') {
				    pipeCount2++;
				}
			}
			if(pipeCount2==0){		
				sing.type = INPUT_TYPE_COMMAND;
				char **args = splitString(inputlar[a], " ");
				command cmd;
				int i;
				for (i = 0; args[i] != nullptr && i < MAX_ARGS - 1; ++i) {
					cmd.args[i] = args[i];
				}
				cmd.args[i] = nullptr; // Ensure the last element is nullptr

				// Copy command into union and single_input
				single_input_union uni;
				uni.cmd = cmd;
				sing.data = uni;
				cmd_input.inputs[a] = sing;
			}
			else{
				sing.type = INPUT_TYPE_PIPELINE;
				char** inputlar2 = splitString(inputlar[a], "|");
				int num_com= pipeCount2 + 1;
				single_input_union uni;
				pipeline plineobj;
				plineobj.num_commands=num_com;
				//implement creating commands
				for (int i = 0; i < num_com; ++i) {
					// Split the command into arguments
					char** args = splitString(inputlar2[i], " ");
					
					// Store the arguments in the command structure
					command cmd;
					int j;
					for (j = 0; args[j] != nullptr; ++j) {
						cmd.args[j] = args[j];
					}
					cmd.args[j] = nullptr; // Mark the end of arguments
					
					// Store the command in the pipeline
					plineobj.commands[i] = cmd;
					
					// Free memory allocated for arguments
					//freeTokens(args);
				}
				uni.pline=plineobj;
				sing.data=uni;
				cmd_input.inputs[a]=sing;
				freeTokens(inputlar2);
				
			}
		}
		main2(&cmd_input);
		freeTokens(inputlar);
	
	}
	
	
}





// Main function
int main() {
    char line[INPUT_BUFFER_SIZE];
    parsed_input input;

    while (1) {
        printf("/> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

        if (strcmp(line, "quit\n") == 0)
            break;

        if (!parse_line(line, &input)) {
            printf("Invalid input.\n");
            continue;
        }
    //pretty_print(&input) ;
/*command cmd;
    cmd.args[0] = "ls";
    cmd.args[1] = "-l";
    cmd.args[2] = NULL; // Null-terminate the argument list

    // Execute the command
    execute_command(&cmd);
    
    // Example pipeline
    command cmd1 = {"ls", "-l", NULL};
    command cmd2 = {"grep", "test", NULL};
    pipeline pline = { {cmd1, cmd2}, 2 };

    // Execute the pipeline
    execute_pipeline(&pline);
    */
        // Execute based on separator
        switch (input.separator) {
            case SEPARATOR_NONE:
                // Execute single command or subshell
                if (input.num_inputs == 1) {
                    switch (input.inputs[0].type) {
                        case INPUT_TYPE_COMMAND:
                            execute_command(&input.inputs[0].data.cmd);
                            break;
                        case INPUT_TYPE_SUBSHELL:
                            execute_subshell(input.inputs[0].data.subshell);
                            //printf("subshell input.\n");
                            break;
                        default:
                            printf("Invalid input.\n");
                            break;
                    }
                } else {
                    printf("Invalid input.\n");
                }
                break;
            case SEPARATOR_PIPE:
            	//printf("pipe input.\n");
				execute_pipeline(&input);
                break;
            case SEPARATOR_SEQ:
	            //printf("seq input.\n");
                execute_sequential(&input);
                
                //if(input.separator == SEPARATOR_PIPE) printf("pipe");                
                //if(input.separator == SEPARATOR_SEQ) printf("seq");
                //if(input.separator == SEPARATOR_PARA) printf("para");  
                break;
            case SEPARATOR_PARA:
                execute_parallel(&input);
                //printf("paralel input.\n");
                //if(input.separator == SEPARATOR_PIPE) printf("pipe");                
                //if(input.separator == SEPARATOR_SEQ) printf("seq");
                //if(input.separator == SEPARATOR_PARA) printf("para");  
                break;
            default:
                printf("Invalid input.\n");
                break;
        }

        free_parsed_input(&input);
    }

    return 0;
}

