#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*********************************Some Explainations about the code************************************/
/*
											id 208685784
				In this assignment we are implementing shell's functionality. the given methods
                that we implement are: "prepare", "process_arglist", "finalize". Additional methods
                were added for code's clarity and organization. As said in the general guidelines in
                the PDF - the different system calls, s.a "dup2", "execvp" were used, and also a 
                mechanism of Ctrl-C was prevented (ignored). we also added a method for reaping 
                children in order that process's sons ended but the father-process didn't get
                the son's return value (zombie processes)
								                                                                      */
/******************************************************************************************************/

int prepare(void);
int process_arglist(int count, char** arglist);
int finalize(void);


void reap_children (int signum)
{
    /*for killing zombies, in case that the father finished
    before sons and the didn't get wait
    in these cases the sons will remain az zombies processes*/
    int e;
	e = errno;
    while (waitpid(-1, 0, WNOHANG) > 0)
	{
	};
    errno = e;
}


void signal_handler_child()
{
    /*method for handling Sigchld
    A handler that will be used in prepare func*/
	char* err = "SigChld Error";
	struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    /*disabeling einter*/
    sig.sa_flags = SA_NOCLDSTOP|SA_RESTART;
    sig.sa_handler = &reap_children;
    if(sigaction(SIGCHLD, &sig, NULL) == -1){
		if(errno != ECHILD && errno != EINTR) {
            /*if error is neither, print error and exit*/
        	fprintf(stderr, err, strerror(errno));
			exit(1);
    	}   
	}
}

void fore_ground_process_signal()
{
    /*foreground processes*/
	char *err = "SigAction Error";
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler =SIG_DFL;
    /*disabeling einter*/
    sig.sa_flags = SA_RESTART;    
    if (sigaction(SIGINT, &sig, NULL)==-1){
        fprintf(stderr, err, strerror(errno));
		exit(1);
    }
}

int proc_id_fork (pid_t * p)
{
	char* err = "Fork Error";
    /*forking*/
    pid_t proc_id = fork();
    if (proc_id < 0)
    /*failed*/
	{
        fprintf(stderr, err , strerror(errno));
        return 1;
    }
    *p = proc_id;
    return 0;
}

int direct_or_background (int count,char** arglist, int background)
{
    /*A method for executing commands direcly of in background
    executing a command in the background uses & at the end of
    the command line*/
	char* err_1 = "Running in Background Error";
	char* err_2 = "Running Error";
	pid_t proc_id;
	if (proc_id_fork(&proc_id))
	{
		return 0;
	}
	if (background)
    /*Running in background*/
	{
		arglist[count-1] = NULL;
		if (!proc_id)
		{
			/*the child execute current command*/
			execvp(arglist[0],arglist);
			fprintf(stderr, err_1 , strerror(errno));
			return 0;
		}
	}
	else 
	{
		if(proc_id)
		{
			waitpid(proc_id, NULL, WUNTRACED);
		}
		else
		{ /*the child execute current command*/
			fore_ground_process_signal();
			execvp(arglist[0],arglist);
			fprintf(stderr, err_2, strerror(errno));
			return 0;
		}
	}
	return 1;
}

int output_redirecting (int count, char** arglist, char* file_to_output)
{
    /*A method for shell's functionality,  shell executes the command
     so that its standard output is redirected to the output file '>'*/
	char* err_1 = "Output File Error Creation";
	char* err_2 = "Dup2 Error";
	char* err_3 = "Execution Error";
    int file_descriptor;
	int proc_id;
    arglist[count-2] = NULL;
    if (proc_id_fork(&proc_id))
	{
        return 0;
    }
    if (proc_id == 0)
	{
        /*foreground process*/
        fore_ground_process_signal();
        file_descriptor = open(file_to_output, O_WRONLY | O_CREAT | O_TRUNC ,0640);
        if (file_descriptor == -1)
		{
            /*error craeting*/
            fprintf(stderr, err_1 , strerror(errno));
            return 0;
        }
        if (dup2 (file_descriptor,1)== -1)
		{
            fprintf(stderr, err_2 , strerror(errno));
            close(file_descriptor);
            return 0;
        } 
        close(file_descriptor);
        execvp(arglist[0],arglist);
        fprintf(stderr, err_3 , strerror(errno));
        return 0;
    }   
    else
	{
        waitpid(proc_id, NULL, WUNTRACED);
    }
    return 1;
}

int sons_pipe_exec(int first_child, int pfds[], char* err)
/*A method for pipe's func. implementing it's sons behavior
 during the pipe procedure*/
{
    /** Foreground process */
        fore_ground_process_signal();
    if (first_child)
    {
        close( pfds[0] ); /*close reading*/
        if (dup2 (pfds[1],1) == -1)
        { /*end writing*/
            fprintf(stderr, err , strerror(errno));
            return 0;
        }
    }
    else 
    {
        if (dup2 (pfds[0],0) == -1) {
            fprintf(stderr, err , strerror(errno)); //dup2 error
            return 0;
        }
    }
    return 1;
}

int process_with_pipe(int count, char** arglist,int ind)
{
    /*A method for shell's functionality. The shell executes both commands concurrently, piping the standard
     output of the first command to the standard input of the second command*/
	char* err_1 = "Exception invoking pipe";
	char* err_2 = "Error executing command";
    pid_t proc_id_1;
    pid_t proc_id_2;
    int pfds[2]; /** Create pipe bitween processes */
    char** origin = arglist;
    char** edited = arglist + ind + 1; /** Split arglist into 2 arglists */
    arglist[ind] = NULL; 
    
    if( -1 == pipe(pfds) )
    {
        fprintf(stderr, err_1 , strerror(errno));
        return 0;
    }
    if (proc_id_fork(&proc_id_1))
    {
        return 0;
    }
    if( proc_id_1 == 0 ) /** first child */
    {
        sons_pipe_exec(1, pfds, err_2);     
        close(pfds[1]);
        execvp(origin[0],origin);
        fprintf(stderr, err_2 , strerror(errno));
        return 0;
    }
    else {/** parent */
        close(pfds[1]); /*close writing*/
        if (proc_id_fork(&proc_id_2)) {
            return 0;
        }
        if( proc_id_2 == 0 ) {/** second child */
            sons_pipe_exec(0, pfds, err_2);          
            close(pfds[0]);
            execvp(edited[0],edited);
            fprintf(stderr, err_2 , strerror(errno));
            return 0;
        }
        else {/** parent */
            waitpid(proc_id_1, NULL, WUNTRACED);
            waitpid(proc_id_2, NULL, WUNTRACED);
        }
    }
    return 1;
}



int prepare(void)
{
    /*As described in the PDF: This function
    returns 0 on success; any other return value indicates an error*/
	char* err = "SigAction Error";
	signal_handler_child();
	struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_flags = SA_RESTART;  
    /*ignore*/
    sig.sa_handler = SIG_IGN;
    if(sigaction(SIGINT, &sig, NULL) == -1)
	{
        fprintf(stderr, err , strerror(errno));
		exit(1);
    }  
	return 0; 
}

int process_arglist(int count, char** arglist)
{
    /*As described in the PDF: This function
    recieves count and argslist, and by those chooses
    how to execute (direcly, background, piping, output-redirecting)*/
	int i;
    if (count >= 1)
	{
		if (arglist[count-1][0] == '&')
		{
            /*backround*/
			return direct_or_background(count,arglist,1);
		}
    }
    if (count >= 2)
	{
		if (arglist[count-2][0] == '>')
		{
            /*output_redirecting*/
			return output_redirecting(count, arglist,arglist[count-1]); 
		}
    }
	i = 0;
    while (i < count-1)
	{
        if (arglist[i][0] == '|')
		{
            /*piping*/
            return process_with_pipe(count, arglist,i);
        }
		i++;
    }
    /*execute directly*/
    return direct_or_background(count,arglist,0);
}


int finalize(void)
{
    /*As described in the PDF: The skeleton calls this function before exiting
    This function returns 0 on success; any other return
    value indicates an error.*/
    return 0;
}
