// An application that pipes the output of command 1 to command 2 and stores
// a copy of the piped data to OUTFILE.
// Usage: ./pipeobserver OUTFILE [ command1 ] [ command2 ]
// Example: ./pipeobserver OUTFILE [ ps aux ] [ grep SCREEN ]
// Example: ./pipobserver OUTFILE [ echo [ Hello ] ] [ wc -l ]
// NOTE: Brackets in cmd line args must be matched. Ex: "[ ls ] ]" will fail.
// NOTE: May be easily adapted to handle an arbitrary number of commands.
//
// Author: Dustin Fast, dustin.fast@hotmail.com, 2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "dstring.h"  // String function lib, uses syscalls only.

#define MAX_LEN (8192)
#define STDIN (STDIN_FILENO)
#define STDOUT (STDOUT_FILENO)
#define STDERR (STDERR_FILENO)
#define USAGE ("./pipeobserver OUTFILE [ EXE ARGS ] [ EXE ARGS ] ...")


// Abstraction of a command and up to MAX_LEN of its cmd line args
typedef struct cmd_obj {
    char exe[MAX_LEN];
    char *args[MAX_LEN];
} command;


// Function defs
void write_err(char *arr);
int get_nextcmd(char **arr_in, int in_len, command *cmd, int i, int j, int k, int z);
void do_pipeconn(int mode, int *pipe_fds, int old_in, int old_out);


// For debug ouput
void debugcmd(char *label, command cmd) {
    str_write("\n", STDOUT);
    str_writeln(label, STDOUT);
    str_writeln(cmd.exe, STDOUT);
    // str_writeln(cmd.args[0], STDOUT);
    // str_writeln(cmd.args[1], STDOUT);
}


// Pipes output between cmd[0] and cmd[1], which are executed as forks -
// Child 1 runs cmd[0], piping it's stdout to GrandchildA which writes it to
// OUTFILE before piping it again to GrandchildB for use as cmd[1]'s stdin.
// Piped data is also copied to the given file desciptor out_fd.
// RETURNS: A pos number denoting number of cmds processed, or a neg err code.
int fork_and_pipe(command *cmds, int cmd_count, int out_fd) {
    command *cmd1, *cmd2;
    pid_t child1_pid, child2_pid, gchildA_pid, gchildB_pid;
    int top_pipe[2];
    int exitcode = -0;  
    int i = 0;

    // Init top-level pipe, to connect child1 stdout to child2 stdin.
    if (pipe(top_pipe) != 0)
        return -1;

    // TODO: for cmd in cmds...  
     while (i < cmd_count) {    
        // Determine the 2 cmds for this iteration
        cmd1 = &cmds[i++];
        cmd2 = &cmds[i++];
        
        child1_pid = fork();
        if (child1_pid == 0) {
            /* In Child 1 ---------------------------------- */
            str_write("Child 1: ", STDOUT); // debug
            str_iwriteln(getpid(), STDOUT); // debug
            close(out_fd);                  // close unused
            close(top_pipe[0]);             // close unused         
            dup2(top_pipe[1], STDOUT);      // redirect stdout
            // TODO: If not firstflag, connect stdin to pipe_read
            exit(execvp(cmd1->exe, cmd1->args));    // do cmd

        } else {
            /* In Parent -------------------------------------------- */            
            waitpid(child1_pid, &exitcode, 0);
            str_write("Child 1 wait done in: ", STDOUT);    // debug
            str_iwriteln(getpid(), STDOUT);                 // debug

            if (exitcode != 0) {
                write_err("ERROR: Invalid cmd1, or cmd1 returned error.");
                return exitcode;
            }

            child2_pid = fork();
            if (child2_pid == 0) {
                /* In Child 2 ------------------------------- */            
                str_write("Child 2: ", STDOUT);     // debug
                str_iwriteln(getpid(), STDOUT);     // debug
                close(top_pipe[1]);                 // close unsued pipe end
                dup2(top_pipe[0], STDIN);           // redirect stdin

                // Init child-level pipe, to pipe gchildA out to gchildB in
                int gchild_pipe[2];
                if (pipe(gchild_pipe) != 0)
                    return -1;

                gchildA_pid = fork();
                if (gchildA_pid != 0) {
                    /* In Grandchild A ------------- */            
                    str_write("GchildA: ", STDOUT);      // debug  
                    str_iwriteln(getpid(), STDOUT);      // debug
                    close(gchild_pipe[0]);               // close unsued
                    dup2(gchild_pipe[1], STDOUT);        // redirect stdout

                    // "tee" stdin to out_fd, and "pipe" data to gchildB
                    char ch;
                    while (read(STDIN, &ch, 1) > 0) {
                        write(out_fd, &ch, 1);
                        write(STDOUT, &ch, 1);

                        // TODO: Hangs without this, but gchildb needs until EOF
                        // if (ch == '\n')
                        //     break;
                    }

                    close(out_fd);
                    sleep(1);  // TODO: debug
                    exit(0);  // exit gchild1

                } else {
                    /* In Child 2 ---------------------------- */ 
                    waitpid(gchildA_pid, &exitcode, 0);
                    str_write("\nGchildA wait done in: ", STDOUT);
                    str_iwriteln(getpid(), STDOUT);

                    if (exitcode != 0) {
                        write_err("ERROR: 'tee' fork exited abnormally.");
                        return exitcode;
                    }
                    
                    gchildB_pid = fork();
                    if (gchildB_pid == 0) {
                        /* In grandchild B ---------- */
                        str_write("\nGchildB: ", STDOUT);
                        str_iwriteln(getpid(), STDOUT);
                        close(out_fd);                  // close unused
                        close(gchild_pipe[1]);          // close unused
                        dup2(gchild_pipe[0], STDIN);    // redirect stdin
                        exit(execvp(cmd2->exe, cmd2->args));    // do cmd
                    }

                    /* Still in Child 2, after GChild A &B --- */ 
                    waitpid(gchildB_pid, &exitcode, 0);
                    str_write("\nGchildB wait done in: ", STDOUT);  // debug
                    str_iwriteln(getpid(), STDOUT);                 // debug

                    if (exitcode != 0)
                        write_err("ERROR: Invalid cmd2, or cmd2 returned error.");

                    exit(exitcode);  // exit child 2
                }

            } else {
                /* Still in Parent --------------------------------- */                            
                waitpid(child2_pid, &exitcode, 0);
                str_write("Child 2 wait done in: ", STDOUT);    // debug
                str_iwriteln(getpid(), STDOUT);                 // debug

                close(top_pipe[0]);
                close(top_pipe[1]);

                if (exitcode != 0) {
                    return(exitcode);
                }

                continue;
                // return i;  // debug
            }
        }
    }
    return i;
}


// Main application driver
int main(int argc, char **argv) {
    command *commands;
    char *outfile_name = argv[1];
    int cmd_count = 0;
    int out_fd;

    // Parse cmd line args for commands to be run
    commands = malloc(MAX_LEN * sizeof(command));
    cmd_count = 0;
    int i = 2;

    while (i < argc && cmd_count > -1) {
        // Get the next cmd from the recursive parser, as a command.
        command cmd;
        i = get_nextcmd(argv, argc, &cmd, i, 0, 0, 0);

        // Set fail, else add cmd to commands
        if (i != -1) 
            commands[cmd_count++] = cmd;
        else 
            cmd_count = -1;
    }

    // Ensure at least two commands
    if (cmd_count < 2) {
        write_err("ERROR: Too few, or invalid cmd arguments received.");
        free(commands);
        return -1;
    }

    // Open the output file
    out_fd = open(outfile_name, O_CREAT | O_WRONLY | O_TRUNC, 00644);
    if (out_fd < 0) {
        write_err("ERROR: Failed to open output file.");
        free(commands);
        return -1;
    }

    int results = fork_and_pipe(commands, cmd_count, out_fd);
    if(results > -1) {
        str_write("Success! ", STDOUT);
        str_iwrite(results, STDOUT);
        str_write(" processes piped.\nUse 'cat ", STDOUT);
        str_write(outfile_name, STDOUT);
        str_writeln("' to view results.", STDOUT);
    } else {
        write_err("ERROR: Failed during fork and pipe process.");
    } 

    close(out_fd);
    free(commands);

    // TODO: Verify no mem leak

    return 0;
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
            // If first open bracket found, next el is exe name. Set z flag.
            if (k == 0)
                z = 1;

            // Else, its an arg.
            else 
                cmd->args[j++] = arr_in[i];

            k++;  // Incr bracket depth
            break;
        
        case ']':
            // If no first open bracket, unmatched bracket error
            if (k == 0)
                return -1;

            // If at bracket level 1, it's our closing bracket, signaling done.
            if (k == 1) {
                cmd->args[j++] = '\0';  // Null terminate args list, for execvp
                return ++i;
            }

            // Else, it's an arg
            cmd->args[j++] = arr_in[i];
            
            k--;  // Decr bracket depth
            break;

        default:
            // If executable name flag set, write cmd.cmd and unset flag.
            if (z) {
                str_cpy(arr_in[i], cmd->exe, str_len(arr_in[i]));
                cmd->args[j++] = arr_in[i];  // First arg is always cmd name
                z = 0;

            // Else, its an arg.
            } else {
                cmd->args[j++] = arr_in[i];
            }
    }

    // Check for unexpected states
    if (k < 0 || (i > in_len && k > 0)) {
        write_err("DEBUG: Unexcepted error in get_nextcmd.");
        return -1;
    }

    // Increase curr str index and recurse
    get_nextcmd(arr_in, in_len, cmd, ++i, j, k, z);
}


// Prints the given char array to stderr with usage info on their own lines.
void write_err(char *arr) {
    str_writeln(arr, STDERR);
    str_write("Usage: ", STDERR);
    str_writeln(USAGE, STDERR);
}
