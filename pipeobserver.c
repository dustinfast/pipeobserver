// This application is a replacement for the "tee" command, implemented 
// using syscalls only. It pipes the output of COMMAND1 to the input of
// COMMAND2, storing a copy of the piped data to the file named by OUTFILE.
//
// Usage: ./pipeobserver OUTFILE [ COMMAND1 ARGS1 ] [ COMMAND2 ARGS2 ]
//    Ex: ./pipeobserver OUTFILE [ ps ] [ grep pts ]
//    Ex: ./pipeobserver OUTFILE [ ps aux ] [ wc ]
//    Ex: ./pipobserver OUTFILE [ echo [ Hello World ] ] [ wc -w ]
//    NOTE: All brackets must be matched. Ex: "[ wc ] ]" will fail.
//
// Author: Dustin Fast, dustin.fast@outlook.com, 2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CMDS (2)                // Denotes max number of commands
#define MAX_LEN (8192)              // Denotes max number of args per command
#define STDIN (fileno(stdin))
#define STDOUT (fileno(stdout))
#define STDERR (fileno(stderr))
#define USAGE ("./pipeobserver OUTFILE [ EXE ARGS ] [ EXE ARGS ]")


// Abstraction of a command, with up to MAX_LEN ARGS
typedef struct cmd_obj {
    char exe[MAX_LEN];
    char *args[MAX_LEN];
} command;


// Main helpers
int get_nextcmd(char **arr_in, int in_len, command *cmd, int i, int j, int k, int z);
void do_pipeconn(int mode, int *pipe_fds, int old_in, int old_out);
int fork_and_pipe(command *cmds, int cmd_count, int outfile_fd);
void write_err(char *arr);

// String helpers
void str_cpy(char *arr_in, char *arr_out, int count);
int str_writeln(char *arr, int fd);
int str_write(char *arr, int fd);
size_t str_len(char *arr);


/* -- Main -- */
int main(int argc, char **argv) {
    command commands[MAX_CMDS];
    char *outfile_name = argv[1];
    int cmd_count = 0;
    int outfile_fd;
    int state = 0;

    // Get up to MAX_CMDS commands, as parsed by the recursive parser
    int i = 2;
    while (i < argc && cmd_count < MAX_CMDS) {
        i = get_nextcmd(argv, argc, &commands[cmd_count++], i, 0, 0, 0);

        // Break on bad cmd format encountered in parser
        if (i < 0) {
            cmd_count = -1;
            break;
        }
    }

    // Ensure at least two valid commands
    if (cmd_count < 2) {
        write_err("Too few, or malformed arguments received.");
        state = -1;
    } else {
        // Open the output file
        outfile_fd = open(outfile_name, O_CREAT | O_WRONLY | O_TRUNC, 00644);
        if (outfile_fd < 0) {
            write_err("Failed to open OUTFILE.");
            state = -1;
        }
    }

    // Do forking and piping of data
    if (state != -1)
        fork_and_pipe(commands, cmd_count, outfile_fd); 

    // Cleanup
    // free(commands[0]);
    close(outfile_fd);

    // TODO: Verify no mem leak

    return state;
}


/* -- get_nextcmd -- */
// Recursively parses **arr_in of form { '[', 'EXE', 'ARGS', ']', ... } and
// populates the given cmd with the next EXE and ARGS, starting at arr_in[i].
// Vars j, k, and z should initially be passed as 0. in_len denotes len(arr_in).
// RETURNS: index of the first unprocessed ptr in arr_in, or -1 if parse error.
int get_nextcmd(char **arr_in, int in_len, command *cmd, int i, int j, int k, int z) {
    // i = curr index of arr_in
    // j = curr index of cmd->args
    // k = denotes open/close bracket depth/level
    // z = denotes next element in arr_in is executable name
    switch(*arr_in[i]) {
        case '[':
            // If first [, next el is exe, so set z flag. Else its an arg.
            if (k == 0)
                z = 1;
            else 
                cmd->args[j++] = arr_in[i];
                
            k++;
            break;
        // [ [ ls -lat ] [ wc -l ];
        case ']':
            // If outter closing bracket, null terminate args & return success.
            if (k == 1) {
                cmd->args[j++] = '\0';
                return ++i;     // <----------------------- Recursive base case
            }

            // If not 1st open bracket, mismatched bracket error.
            if (k <= 0)
                return -1;
            
            //Else, it's an arg.
            cmd->args[j++] = arr_in[i];
            
            k--;
            break;

        default:
            // If executable name flag set, write cmd->cmd and unset z flag.
            // Note that first argument is always exe name, so set that as well.
            if (z) {
                str_cpy(arr_in[i], cmd->exe, str_len(arr_in[i]));
                cmd->args[j++] = arr_in[i];  // Note: j always be 0 here
                z = 0;
                break;
            }

            // Else, its an arg.
            cmd->args[j++] = arr_in[i];
    }

    // If still in bounds of arr_in for next iteration, recurse, else err.
    i++;
    if (i >= in_len)
        return -1;
    else  
        get_nextcmd(arr_in, in_len, cmd, i, j, k, z);
}

/* -- fork_and_pipe -- */
// Pipes output between cmd[0] and cmd[1], which are executed as subprocesses.
// Child 1 runs cmd[0], piping it's stdout to GrandchildA which writes it to
// OUTFILE before piping it again to GrandchildB for use as cmd[1]'s stdin.
// Piped data is also copied to the given file desciptor, outfile_fd.
// RETURNS: 0 on success, else a negative integer.
int fork_and_pipe(command *cmds, int cmd_count, int outfile_fd) {
    command *cmd1 = &cmds[0];
    command *cmd2 = &cmds[1];
    pid_t child1_pid, child2_pid, gchildA_pid, gchildB_pid;
    int top_pipe[2];
    int exitcode = 0;

    // Init top-level pipe, to connect child1 stdout to child2 stdin.
    if (pipe(top_pipe) != 0)
        return -1;
    
    /* In Parent, before any forking ------------------------------------ */            
    child1_pid = fork();
    if (child1_pid == 0) {
        /* In Child 1 ---------------------------------- */
        close(outfile_fd);                      // close unused
        close(top_pipe[0]);                     // close unused         
        dup2(top_pipe[1], STDOUT);              // redirect stdout
        execvp(cmd1->exe, cmd1->args);          // do cmd
        exit(-1);                               // if here, error w/cmd1

    } else {
        /* In Parent, after Child 1 fork -------------------------------- */            
        waitpid(child1_pid, &exitcode, 0);
        close(top_pipe[1]);                     // Done with this end        
        
        if (exitcode != 0) {
            write_err("Invalid cmd1, or cmd1 returned error.");
            return exitcode;
        }

        child2_pid = fork();
        if (child2_pid == 0) {
            /* In Child 2 ------------------------------- */            
            dup2(top_pipe[0], STDIN);           // redirect stdin

            // Init child-level pipe, to pipe gchildA stdout to gchildB stdin
            int gchild_pipe[2];
            if (pipe(gchild_pipe) != 0)
                return -1;

            gchildA_pid = fork();
            if (gchildA_pid != 0) {
                /* In Grandchild A ------------- */            
                close(gchild_pipe[0]);           // close unsued
                dup2(gchild_pipe[1], STDOUT);    // redirect stdout

                // "tee" stdin to outfile_fd, and "pipe" data to gchildB
                char ch;
                while (read(STDIN, &ch, 1) > 0) {
                    write(outfile_fd, &ch, 1);
                    write(STDOUT, &ch, 1);
                }

                close(outfile_fd);
                exit(0);

            } else {
                /* In Child 2, after GchildA -------------- */ 
                waitpid(gchildA_pid, &exitcode, 0);

                if (exitcode != 0) {
                    write_err("'tee' fork exited abnormally.");
                    return exitcode;
                }
                
                gchildB_pid = fork();
                if (gchildB_pid == 0) {
                    /* In grandchild B ---------- */
                    close(outfile_fd);              // close unused
                    close(gchild_pipe[1]);          // close unused
                    dup2(gchild_pipe[0], STDIN);    // redirect stdin
                    execvp(cmd2->exe, cmd2->args);  // do cmd
                    exit(-1);                       // if here, error w/cmd2

                }
                else { exit(0); }                   // Exit child 2
            }

        } else {
            /* In Parent, after child2 fork ----------------------------- */  
            waitpid(gchildB_pid, &exitcode, 0);                          
            waitpid(child2_pid, NULL, 0);
            close(top_pipe[0]);

            return(exitcode);
        }
    }
}

/* -- write_err -- */
// Writes the given null-terminated string with program usage info to stderr.
void write_err(char *arr) {
    str_write("ERROR: ", STDERR);
    str_writeln(arr, STDERR);
    str_write("TRY: ", STDERR);
    str_writeln(USAGE, STDERR);
}


/* -- str_len -- */
// Returns the length of the given null-terminated "string".
size_t str_len(char *arr) {
    int length = 0;
    for (char *c = arr; *c != '\0'; c++)
        length++;

    return length;
}


/* -- str_write -- */
// Writes the string given by arr to filedescriptor fd.
// RETURNS: The number of bytes written, or -1 on error.
int str_write(char *arr, int fd) {
    size_t curr_write = 0;
    size_t total_written = 0;
    size_t char_count = str_len(arr);

    // If bad fd
    if (fd < 0) {
        str_write("Invalid file descriptor passed to str_write", STDERR);
        return -1;
    }

    // If empty string , do nothing
    if (!char_count) 
        return 0;

    // Write string to the given file descriptor (note ptr arith in write()).
    while (total_written < char_count) {
        curr_write = write(fd, arr + total_written, char_count - total_written);
        
        if (curr_write < 0)
            return -1; // on error
        total_written += curr_write;
    }

    return total_written;
}


/* -- str_writeln -- */
// Writes the given string to filedescriptor fd, followed by a newline.
int str_writeln(char *arr, int fd) {
    str_write(arr, fd);
    str_write("\n", fd);
}


/* -- str_cpy -- */
// Copies the given number of bytes from arr_in to arr_out.
// ASSUMES: arr_out is of sufficient size.
void str_cpy(char *arr_in, char *arr_out, int count) {
    for (int i = 0; i < count; i++) {
            arr_out[i] = arr_in[i];
    }
}
