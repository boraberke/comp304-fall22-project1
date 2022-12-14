#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#define NUM_OF_BUILTIN_CMDS 6
#define ROOM_CAPACITY 20
const char * sysname = "shellax";

enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection
	struct command_t *next; // for piping
	int pipe_read_end; // for piping
};

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t * command)
{
	int i=0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background?"yes":"no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete?"yes":"no");
	printf("\tRedirects:\n");
	for (i=0;i<3;i++)
		printf("\t\t%d: %s\n", i, command->redirects[i]?command->redirects[i]:"N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i=0;i<command->arg_count;++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}
/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i=0; i<command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i=0;i<3;++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next=NULL;
	}
	free(command->name);
	free(command);
	return 0;
}
/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}
/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters=" \t"; // split at whitespace
	int index, len;
	len=strlen(buf);
	while (len>0 && strchr(splitters, buf[0])!=NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len>0 && strchr(splitters, buf[len-1])!=NULL)
		buf[--len]=0; // trim right whitespace

	if (len>0 && buf[len-1]=='?') // auto-complete
		command->auto_complete=true;
	if (len>0 && buf[len-1]=='&') // background
		command->background=true;

	char *pch = strtok(buf, splitters);
	command->name=(char *)malloc(strlen(pch)+1);
	if (pch==NULL)
		command->name[0]=0;
	else
		strcpy(command->name, pch);

	command->args=(char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index=0;
	char temp_buf[1024], *arg;
	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch) break;
		arg=temp_buf;
		strcpy(arg, pch);
		len=strlen(arg);

		if (len==0) continue; // empty arg, go for next
		while (len>0 && strchr(splitters, arg[0])!=NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len>0 && strchr(splitters, arg[len-1])!=NULL) arg[--len]=0; // trim right whitespace
		if (len==0) continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|")==0)
		{
			struct command_t *c=malloc(sizeof(struct command_t));
			int l=strlen(pch);
			pch[l]=splitters[0]; // restore strtok termination
			index=1;
			while (pch[index]==' ' || pch[index]=='\t') index++; // skip whitespaces

			parse_command(pch+index, c);
			pch[l]=0; // put back strtok termination
			command->next=c;
			continue;
		}

		// background process
		if (strcmp(arg, "&")==0)
			continue; // handled before

		// handle input redirection
		redirect_index=-1;
		if (arg[0]=='<')
			redirect_index=0;
		if (arg[0]=='>')
		{
			if (len>1 && arg[1]=='>')
			{
				redirect_index=2;
				arg++;
				len--;
			}
			else redirect_index=1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index]=malloc(len);
			strcpy(command->redirects[redirect_index], arg+1);
			continue;
		}

		// normal arguments
		if (len>2 && ((arg[0]=='"' && arg[len-1]=='"')
					|| (arg[0]=='\'' && arg[len-1]=='\''))) // quote wrapped arg
		{
			arg[--len]=0;
			arg++;
		}
		command->args=(char **)realloc(command->args, sizeof(char *)*(arg_index+1));
		command->args[arg_index]=(char *)malloc(len+1);
		strcpy(command->args[arg_index++], arg);
	}
	command->pipe_read_end = -1; //default to -1 for checks
	command->arg_count=arg_index;
	return 0;
}

void prompt_backspace()
{
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}
/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index=0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
						 // Those new settings will be set to STDIN
						 // TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


	//FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state=0;
	buf[0]=0;
	while (1)
	{
		c=getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c==9) // handle tab
		{
			buf[index++]='?'; // autocomplete
			break;
		}

		if (c==127) // handle backspace
		{
			if (index>0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c==27 && multicode_state==0) // handle multi-code keys
		{
			multicode_state=1;
			continue;
		}
		if (c==91 && multicode_state==1)
		{
			multicode_state=2;
			continue;
		}
		if (c==65 && multicode_state==2) // up arrow
		{
			int i;
			while (index>0)
			{
				prompt_backspace();
				index--;
			}
			for (i=0;oldbuf[i];++i)
			{
				putchar(oldbuf[i]);
				buf[i]=oldbuf[i];
			}
			index=i;
			continue;
		}
		else
			multicode_state=0;

		putchar(c); // echo the character
		buf[index++]=c;
		if (index>=sizeof(buf)-1) break;
		if (c=='\n') // enter key
			break;
		if (c==4) // Ctrl+D
			return EXIT;
	}
	if (index>0 && buf[index-1]=='\n') // trim newline from the end
		index--;
	buf[index++]=0; // null terminate string

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	//print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

// a struct to hold count, line, size of line and the next line in a linked list 
struct uniq_t {
	int c; //count
	char* l; //line
	ssize_t s; //size of line
	struct uniq_t *next; //next elt in linked list
};

// used to construct a new uniq_t from given line and size
struct uniq_t* cons_new_uniq_t(struct uniq_t* u, char *line,ssize_t nread){
	u->l = (char *)malloc(sizeof(char) * nread);
	strcpy(u->l,line);
	u->c = 1;
	u->s = nread;
	u->next = NULL;
	return u;
}
//part 3 - implementation of uniq as a builtin command
int uniq(char ** args){

	//handle argument -c or --count
	bool printCount = false;
	if (args[1]!=NULL){
		if((strcmp(args[1],"--count")==0) || (strcmp(args[1], "-c")==0)){
			printCount = true;
		}
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	struct uniq_t *prev_line = malloc(sizeof(struct uniq_t));
	prev_line->l = NULL;
	prev_line->c = 0;

	struct uniq_t **unique_lines;
	memcpy(&prev_line,unique_lines,sizeof(&prev_line));

	while ((nread = getline(&line, &len, stdin)) != -1) {
		//check the last line, if it is same with the current line, increase the count
		//	else if not the same, save the last line and count to a list and 
		//	change the last line to current line with count 1

		if (prev_line->l == NULL){
			//i.e. first element
			prev_line = cons_new_uniq_t(prev_line,line, nread);
		}else if (strcmp(prev_line->l, line) == 0){
			// element exists, then increase count
			prev_line->c += 1;
		}else{
			//add the newline as next update prev_line with next_line
			struct uniq_t *next_line = malloc(sizeof(struct uniq_t));
			next_line = cons_new_uniq_t(next_line,line, nread);
			prev_line->next = next_line;
			prev_line = next_line;
		}	
	}
	free(line);

	// print out the result
	while ((*unique_lines)->next != NULL){
		*unique_lines = (*unique_lines)->next;
		if(printCount){
			printf("\t%d %s",(*unique_lines)->c,(*unique_lines)->l);
		}else{
			printf("%s",(*unique_lines)->l);
		}
	}

}

int chatroom(char ** args){
	//disable buffer in stdout
	setbuf(stdout,NULL);

	//check if args are given correctly
	if (args[1] ==NULL || args[2]==NULL){
		printf("Invalid syntax, correct syntax: chatroom <room> <username>\n");
		exit(1);
	}

	int buf_size = 512;

	char *tmp = "/tmp/";

	int len = strlen(args[1]);
	char* room_name = malloc(len*sizeof(char));
	strcpy(room_name, args[1]);

	len = strlen(args[2]);
	char* user_name = malloc(len*sizeof(char));
	strcpy(user_name, args[2]);

	char room_path[buf_size];
	snprintf(room_path, sizeof(room_path), "%s%s/", tmp, room_name);	

	char user_path[buf_size];
	snprintf(user_path, sizeof(user_path), "%s%s", room_path, user_name);

	//create room and pipe if does not exist.
	DIR* room= opendir(room_path);
	struct dirent *dir;

	int i = 0;
	if (room) {
		printf("Welcome to %s!\n",room_name);
		closedir(room);
	} else if (ENOENT == errno) {
		/* room does not exist, create one. */
		mkdir(room_path,0777);
	} else {
		/* failed for some other reason. */
		printf("problem with opening directory.\n");
		exit(1);
	}


	int pipe_check = mkfifo(user_path, 0666);
	if (pipe_check && EEXIST != errno) {
		/* failed for some other reason. */
		printf("problem with named pipe.\n");
		exit(1);
	}

	//start reading from pipe and writing to other pipes.
	int fd1;
	char r_end[buf_size*2], w_end[buf_size*2];

	if(fork() == 0){
		while(1){
			//read all the users in the room
			DIR* room= opendir(room_path);
			struct dirent *dir;

			printf("%s: <write your message>",user_name);
			fgets(w_end, buf_size*2, stdin);

			// add room name and user name in front
			char message_to_send[buf_size*3];
			snprintf(message_to_send, sizeof(message_to_send), "[%s] %s: %s", room_name, user_name, w_end);

			//print your sent message here	
			printf("\033[A\r%*s",250,"");
			printf("\033[A\r%s",message_to_send);
			if (room) {
				while ((dir = readdir(room)) != NULL) {
					if(dir->d_type == DT_FIFO){
						if(strcmp(dir->d_name,user_name)==0){
							continue;
						}
						//open fifo and write the message
						char *fifo_path = malloc(buf_size);
						snprintf(fifo_path,buf_size,"%s%s",room_path,dir->d_name);
						fd1 = open(fifo_path,O_WRONLY);
						write(fd1, message_to_send, strlen(message_to_send)+1);
						close(fd1);

					}
				}
				closedir(room);
			} else {
				/* failed for some other reason. */
				printf("problem with opening directory.\n");
				exit(1);
			}

		}
	}

	while (1)
	{
		fd1 = open(user_path,O_RDONLY);
		read(fd1, r_end, buf_size*2);
		// Print the read and close 
		printf("\r%*s",250,"");
		printf("\033[A\r%s",r_end);
		printf("%s: <write your message>",user_name);
		close(fd1);
	}

}
int wiseman(char ** args) {
	int interval = atoi(args[1]);
	char buff[50];
	//preparing the text file to send over to crontab
	sprintf(buff, "*/%d * * * * fortune | espeak\n", interval);
	char* file_name = "crontab.txt";
	FILE* file = fopen(file_name, "w");
	fprintf(file, "%s", buff);
	fclose(file);
	execlp("crontab", "crontab", file_name, NULL);

}

int dance(char ** args) {

	int type = atoi(args[1]);
	int count = atoi(args[2]) - 1;
	int i;
	char dance_l[50];
	char dance_r[50];
	
	setbuf(stdout, NULL); //the printfs wouldn't work properly.
	printf("??? ??? ???_??? ??????    ");
	sleep(1);
	printf("(\r ?????? ???????????? ???)???    ");
	sleep(1);
	if (type == 1) {
		strcpy(dance_l, "(?????? ?????? )???*:?????????     ");
		strcpy(dance_r, "?????????: *???(??? ?????????)   ");
	}

	if (type == 2) {
		strcpy(dance_l, "(~???????)~            ");
		strcpy(dance_r, "~(???????~)            ");
	}

	if (type == 3) {
		strcpy(dance_l, "???(?????????)???            ");
		strcpy(dance_r, "???(?????????)???            ");
	}

	printf("\r%s", dance_l);
	for (i = 0; i < count; i++) {

		printf("\r%s", dance_r);
		sleep(1);
		printf("\r%s", dance_l);
		sleep(1);

	}
	printf("\r%s", dance_r);
	sleep(1);
	printf("\r???(?????? _??? )??????       \n");
	sleep(1);
}
int snake(char ** args){
	execv("snake", args);
}
int psvis(char ** args){
	char buff1[50];
	sprintf(buff1, "sudo dmesg -C; sudo insmod psvis.ko PID=%d", atoi(args[1]));
	system(buff1);
	system("sudo rmmod psvis");
	//draw
	FILE* data = fopen("data.txt", "w");
	system("echo \"graph {\n\" > data.txt; sudo dmesg | cut -d [ -f2- | cut -d ] -f2- | grep 'Start time' >> data.txt; echo \"}\n\" >> data.txt");
	char buff2[50];
	sprintf(buff2, "cat data.txt | dot -Tpng > %s.png", args[2]);
	system(buff2);

}

typedef int (*builtin_cmd) (char **);
builtin_cmd builtin_cmds[NUM_OF_BUILTIN_CMDS] =  {&uniq, &chatroom, &wiseman, &dance, &snake, &psvis};
const char *builtin_cmd_names[NUM_OF_BUILTIN_CMDS] = {"uniq", "chatroom", "wiseman", "dance", "snake", "psvis"};


int process_command(struct command_t *command);
int main()
{
	while (1)
	{
		struct command_t *command=malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code==EXIT) break;

		code = process_command(command);
		if (code==EXIT) break;

		free_command(command);
	}

	printf("\n");
	return 0;
}
int exec_cmd(struct command_t *command){
	//redirection for '<'
	// input is read from a file
	int fd[2];
	if (command->redirects[0] != NULL){
		fd[0] = open(command->redirects[0], O_RDONLY);
		dup2(fd[0],STDIN_FILENO);
		close(fd[0]);
	}
	//redirection for '>'
	// output file is created if does not exists
	// truncated if it does
	if (command->redirects[1] != NULL){
		fd[1] = open(command->redirects[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
		dup2(fd[1],STDOUT_FILENO);
		close(fd[1]);
	}	
	//redirection for '>>'
	// output file is created if does not exists
	// appended if it does
	if (command->redirects[2] != NULL){
		fd[1] = open(command->redirects[2], O_WRONLY|O_CREAT|O_APPEND, 0644);
		dup2(fd[1],STDOUT_FILENO);
		close(fd[1]);
	}

	// This shows how to do exec with environ (but is not available on MacOs)
	// extern char** environ; // environment variables
	// execvpe(command->name, command->args, environ); // exec+args+path+environ

	/// This shows how to do exec with auto-path resolve
	// add a NULL argument to the end of args, and the name to the beginning
	// as required by exec

	// increase args size by 2
	command->args=(char **)realloc(
			command->args, sizeof(char *)*(command->arg_count+=2));

	// shift everything forward by 1
	for (int i=command->arg_count-2;i>0;--i)
		command->args[i]=command->args[i-1];

	// set args[0] as a copy of name
	command->args[0]=strdup(command->name);
	// set args[arg_count-1] (last) to NULL
	command->args[command->arg_count-1]=NULL;

	// check if the command is a builtin cmd.
	for(int i=0;i < NUM_OF_BUILTIN_CMDS; i++){
		if(strcmp(command->name,builtin_cmd_names[i])==0){
			// call the function of the command instead, then exit	
			builtin_cmds[i](command->args);
			exit(0);
		}
	}
	//finding the path of a program
	char true_path[80];
	//if the program is in the same directory no need for looking for env paths
	char dr[3];
	dr[0] = command->name[0];
	dr[1] = command->name[1];
	dr[2] = '\0';
	if (!strcmp(dr,"./")) {

		strcpy(true_path, command->name);

	} 
	//if the program is NOT in the same directory, look for env paths
	else {
		char* buffer = getenv("PATH");
		int len = strlen(buffer);
		char* path = malloc(len*sizeof(char));
		strcpy(path, buffer);
		char* token = strtok(path, ":");

		char* paths[500];
		int i = 0;
		while (token != NULL) {
			paths[i] = token;
			i++;
			token = strtok(NULL, ":");
		}
		free(path);
		paths[i] = "0";
		//iterating over each path to find the program
		int j = 0;
		while (strcmp(paths[j],"0")) {
			char full_path[80] = "";
			strcat(full_path,paths[j]);
			strcat(full_path,"/");
			strcat(full_path,command->name);

			int code = access(full_path, X_OK);
			if (!code) {
				strcpy(true_path, full_path);
				break;
			}
			j++;
		}
	}

	int err = execv(true_path, command->args);
	if (err == -1) {
		printf("\nCommand or program not found.\n");
	}
	exit(0);
	/// TODO: do your own exec with path resolving using execv()

}
int pipe_redirect_cmd(struct command_t *command){

	int fd[2];
	while(command->next != NULL){
		pipe(fd);
		if (fork()==0){

			// if it is not the first cmd of the piped expression (i.e. in the middle)
			// use previous pipe_read_end to read from
			if (command->pipe_read_end!= -1 ){
				//redirect stdin to pipe_read_end
				dup2(command->pipe_read_end, STDIN_FILENO);
				close(command->pipe_read_end);	
			}	

			//redirect stdout to pipe_write
			dup2(fd[1],STDOUT_FILENO);
			close(fd[1]);

			exec_cmd(command);
			return SUCCESS;
		}

		close(command->next->pipe_read_end);
		close(fd[1]);

		// give pipe_read to command->next
		command->next->pipe_read_end = fd[0];
		command = command->next;
	}

	// last command of piped expression (if exists)
	if (command->pipe_read_end != -1){
		//redirect stdin to pipe_read_end
		dup2(command->pipe_read_end, STDIN_FILENO);
		close(command->pipe_read_end);
	}

	// if there is no pipe, then code will directly follow here
	exec_cmd(command);



}
int process_command(struct command_t *command)
{
	int r;
	if (strcmp(command->name, "")==0) return SUCCESS;

	if (strcmp(command->name, "exit")==0)
		return EXIT;

	if (strcmp(command->name, "cd")==0)
	{
		if (command->arg_count > 0)
		{
			r=chdir(command->args[0]);
			if (r==-1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}

	pid_t pid=fork();
	if (pid==0) // child
	{
		pipe_redirect_cmd(command);
	}
	else
	{
		if(!command->background){
			// TODO: implement background processes here
			int status;
			waitpid(pid, &status, 0); // wait for child process to finish if it's not background
		}
		return SUCCESS;

	}

	// TODO: your implementation here

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}
