// An application that pipes the output of command 1 to command 2 and stores
// a copy of the piped data to OUTFILE.
// Usage: ./pipeobserver OUTFILE [ command1 ] [ command2 ]
// Example: ./pipeobserver OUTFILE [ ps aux ] [ grep SCREEN ]
// Example: ./pipobserver OUTFILE [ echo [ Hello ] ] [ wc -l ]
// NOTE: Brackets in cmd line args must be matched. Ex: "[ ls ] ]" will fail.
//
// Author: Dustin Fast, dustin.fast@hotmail.com, 2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "dstring.h"            // String function lib, uses syscalls only.

#define MAX_LEN (8192)          // Denotes max number of args per command
#define MAX_CMDS (2)            // Denotes max number of commands
#define STDIN (STDIN_FILENO)    // Time...
#define STDOUT (STDOUT_FILENO)  // Is...
#define STDERR (STDERR_FILENO)  // Money.
#define USAGE ("./pipeobserver OUTFILE [ EXE ARGS ] [ EXE ARGS ]")


// Abstraction of a command having up to MAX_LEN cmd line args
typedef struct cmd_obj {
    char exe[MAX_LEN];
    char *args[MAX_LEN];
} command;


// Function defs
void write_err(char *arr);
int get_nextcmd(char **arr_in, int in_len, command *cmd, int i, int j, int k, int z);
void do_pipeconn(int mode, int *pipe_fds, int old_in, int old_out);
int fork_and_pipe(command *cmds, int cmd_count, int outfile_fd);


// Main
int main(int argc, char **argv) {
    command *commands;
    char *outfile_name = argv[1];
    int cmd_count = 0;
    int outfile_fd;
    int state = 0;

    // Parse cmd line args for commands to be run
    commands = malloc(MAX_LEN * sizeof(command));
    cmd_count = 0;
    int i = 2;

    // Get commands, as parsed by the recursive parser
    while (i < argc && cmd_count < MAX_CMDS) {
        command *cmd = malloc(sizeof(command));
        i = get_nextcmd(argv, argc, cmd, i, 0, 0, 0);

        // Break on bad cmd format encountered in parser
        if (i < 0) 
            break;
        commands[cmd_count++] = *cmd;
    }

    // Ensure at least two valid commands
    if (cmd_count < 2) {
        write_err("ERROR: Too few, or malformed arguments received.");
        state = -1;
    } else {
        // Open the output file
        outfile_fd = open(outfile_name, O_CREAT | O_WRONLY | O_TRUNC, 00644);
        if (outfile_fd < 0) {
            write_err("ERROR: Failed to open output file.");
            state = -1;
        }
    }

    // Do forking and piping of data
    if (state != -1)
        fork_and_pipe(commands, cmd_count, outfile_fd); 

    // Cleanup
    free(commands);
    close(outfile_fd);

    // TODO: Verify no mem leak

    return state;
}


// Recursively parses char** of form { '[', 'EXE', 'ARGS', ']', ... } and
// populates the given cmd with the next EXE and ARGS, starting at arr_in[i].
// Vars j, k, and z should initially be passed as 0. in_len denotes len(arr_in).
// RETURNS: index of the first unprocessed ptr in arr_in, or -1 if parse error.
int get_nextcmd(char **arr_in, int in_len, command *cmd, int i, int j, int k, int z) {
    // i = curr index of arr_in
    // j = curr index of cmd->args
    // k = denotes open/close bracket depth/level
    // z = denotes next arg is executable name
    switch(*arr_in[i]) {
        case '[':
            // If first [, next el is exe, so set z flag. Else its an arg.
            if (k == 0)
                z = 1;
            else 
                cmd->args[j++] = arr_in[i];
            k++;
            break;
        
        case ']':
            // If not first open bracket, unmatched bracket error
            if (k == 0)
                return -1;

            // If outter closing bracket, null terminate args & return
            if (k == 1) {
                cmd->args[j++] = '\0';
                return ++i;
            }

            // Else, it's an arg
            cmd->args[j++] = arr_in[i];
            
            k--;
            break;

        default:
            // If executable name flag set, write cmd.cmd and unset flag.
            if (z) {
                str_cpy(arr_in[i], cmd->exe, str_len(arr_in[i]));
                cmd->args[j++] = arr_in[i];  // First arg is cmd name (j=0)
                z = 0;
                break;
            }

            // Else, its an arg.
            cmd->args[j++] = arr_in[i];
    }

    // Increase curr str index and recurse
    get_nextcmd(arr_in, in_len, cmd, ++i, j, k, z);
}


// Pipes output between cmd[0] and cmd[1], which are executed as forks -
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
            write_err("ERROR: Invalid cmd1, or cmd1 returned error.");
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
                    write_err("ERROR: 'tee' fork exited abnormally.");
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

// Prints the given char array and usage info to stderr on their own lines.
void write_err(char *arr) {
    str_writeln(arr, STDERR);
    str_write("Usage: ", STDERR);
    str_writeln(USAGE, STDERR);
}
