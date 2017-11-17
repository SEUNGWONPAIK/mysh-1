#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio_ext.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "commands.h"
#include "built_in.h"
#include "utils.h"


#define SOCK_PATH  "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
}; 

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);//numbers of array elements

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;// if command_name is possible  
    }  
  }

  return -1; // Not found
}

void* server(void ***argv)  {

	int server_sock, client_sock, len, rc;
	int bytes_rec = 0;
	int pid,status;
	char buf[256];
	int backlog = 10;
	char ***arg=(char***)argv;
	
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;         

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(buf, 0, 256);                

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0); //create server socket 
	if (server_sock == -1){
	printf("SOCKET ERROR");
	exit(1);
	}

	server_sockaddr.sun_family = AF_UNIX;  //inter process communication
	strcpy(server_sockaddr.sun_path, SOCK_PATH);  //path copy
	len = sizeof(server_sockaddr);  //adress size

	unlink(SOCK_PATH);  //link delete
	rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);// ready for use socket
	if (rc == -1){
	printf("BIND ERROR");
	close(server_sock);
	exit(1);
	}

	rc = listen(server_sock, backlog);  //waiting for clients
	if (rc == -1){ 
	printf("LISTEN ERROR");
	close(server_sock);
	exit(1);
	}

	// make a socket for communication with client
	client_sock = accept(server_sock, (struct sockaddr*)&client_sockaddr,    /*(socklen_t*)*/&len);
	if (client_sock == -1){
	printf("ACCEPT ERROR");
	close(server_sock);
	close(client_sock);
	exit(1);
	}

	if((pid=fork())<0)
	perror("fork is failed!!\n");
	else if(pid != 0) //parent
	sleep(2);
	else{ //child
	close(0);  //close  input
	int a=dup(client_sock);//duplicate client socket
	close(client_sock);
	execv(arg[0][0],arg[0]);
	close(a);
	}     

	close(server_sock);
	close(client_sock);
    
}


void* client(void ***argv){	

	int client_sock, rc, len;
	int pid,status;
	char ***arg=(char***)argv;

	struct sockaddr_un server_sockaddr; 
	struct sockaddr_un client_sockaddr; 
	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
	
	// create socket
	client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_sock == -1) {
	printf("SOCKET ERROR");
	exit(1);
	}

	client_sockaddr.sun_family = AF_UNIX;   
	strcpy(client_sockaddr.sun_path, CLIENT_PATH); 
	len = sizeof(client_sockaddr);

	// delete link
	unlink(CLIENT_PATH);
	rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
	if (rc == -1){
	printf("BIND ERROR");
	close(client_sock);
	exit(1);
	}

	//initial socket struct & connect to server	
	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SERVER_PATH);
	rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len);
	if(rc == -1){
	printf("CONNECT ERROR");
	close(client_sock);
	exit(1);
	}

	if((pid=fork())<0)
	perror("fork is failed!!\n");
	else if(pid != 0) //parent
	sleep(2);
	else{ //child	
	close(1); //close output
	int a=dup(client_sock);
	close(client_sock);
	execv(arg[0][0],arg[0]);
	close(a);
	}

	close(client_sock);

}


/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
   if (n_commands > 0) {
	
	struct single_command* com = (*commands);
	assert(com->argc != 0);
	int built_in_pos = is_built_in_command(com->argv[0]);
	int pid,status,ex=0;
    
    if (built_in_pos != -1) { //error check if clauses
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } 

    else   {	//normal instruction
		if(n_commands>=2){
			
			int pid,status,thr_id;
			pthread_t p_thread[2];
						
			for(int i=1;i<n_commands;i++){

				if((pid=fork())<0)
				perror("fork is failed!!\n");

				else if(pid != 0){ //parents
					com=(*commands)+i;	
					thr_id=pthread_create(&p_thread[0],NULL,&server,(void***)&(com->argv));
					sleep(4);
					pthread_join(p_thread[0],NULL);
				}

				else{ // child
				thr_id=pthread_create(&p_thread[1],NULL,&client,(void***)&(com->argv));
				sleep(1);
				}//else

	
			}//for
	}//else

		else{// command 1
			for(int i=0;i<n_commands;i++){
				if((pid=fork())<0)
				perror("fork is failed!!\n");
				else if(pid != 0) //parents
				pid=wait(&status);
				else{ // child
				ex=execv(com->argv[0],com->argv);
				if(ex==-1){		
				fprintf(stderr, "%s: command not found\n", com->argv[0]);
				return -1;
				}

				}
				}	//else
	
			}	//for
 		  }

}
  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
