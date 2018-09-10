// An application that pipes one processes stdout to the stdin of another, 
// and writing a copy of the data piped to the file given by outfile.
// Usage: ./pipeobserver outfile [ process1 ] [ process2 ]
// Example: ./pipeobserver outfile [ ps aux ] [ process2 ]
// Example: ./pipobserver outfile [ echo [ Hello ] ] [ wc -l ]
// NOTE: Brackets in cmd line args must be matched. Ex: "[ hi ]]" is incorrect.
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
#define USAGE ("./pipeobserver OUTFILE [ CMD1 ] [ CMD2 ]")


// Command representation
typedef struct cmd_obj {
    char cmd[MAX_LEN];
    char args[MAX_LEN];
} command;


// Func defs
void write_err(char *arr);
int parse_cmds(char **arr_in, char *arr_out, int in_len, int i, int j, int k);
int get_cmds(int argc, char **argv, command *cmds);


// For debug
void debugcmd(char *label, command cmd) {
    str_write("\n", STDOUT);
    str_writeln(label, STDOUT);
    str_writeln(cmd.cmd, STDOUT);
    str_writeln(cmd.args, STDOUT);
    str_write("\n", STDOUT);
}


// Helper function to connect read/write/close pipes.
// Mode 1: replace old_in w/ pipe_fds[1]
// Mode 2: replace old_out w/ pipe_fds[0]
// Mode 3: Does both  mode 1 and mode 2.
// old_in and/or old_out may be NULL, depending on mode.
void connect_pipe(int mode, int *pipe_fds, int old_in, int old_out) {
    switch(mode) {
        case 1:
            dup2(pipe_fds[1], old_out);
            close(pipe_fds[1]);
            break;
        case 2:
            dup2(pipe_fds[0], old_in);
            close(pipe_fds[0]);
            break;
        case 3:
            dup2(pipe_fds[1], old_out);
            dup2(pipe_fds[0], old_in);
            break;
        default:
            str_writeln("ERROR: Invalid mode passed to connect_pipe", STDERR);
    }
}


// Pipes output between the given CMDS which are executed as forks.
// Piped data is copied to the given file desciptor out_fd.
int fork_and_pipe(command *cmds, int cmd_count, int out_fd) {
    command *cmd1, *cmd2;
    pid_t cmd1_pid, tee_pid, cmd2_pid;
    int top_pipe[2];
    int i = 0;

    // Init top-level pipe
    if (pipe(top_pipe) != 0)
        return -1;

    // for cmd in cmds...
     while (i < cmd_count) {    
        // Determine the 2 cmds for this iteration
        cmd1 = &cmds[i++];
        cmd2 = &cmds[i++];
        
        cmd1_pid = fork();
        if (cmd1_pid == 0) {
            // In Child 1 ...
            debugcmd("Start cmd 1:", *cmd1);  // debug
            close(out_fd); 
                            
            // TODO: If not firstflag, connect stdin to pipe_read
            char* exec_args[] = {cmd1->args, NULL};
            execvp(cmd1->cmd, exec_args); // TODO: args not well-formed

            exit(0);  // exit cmd1_pid

        } else {
            // In Parent ...
            waitpid(cmd1_pid, NULL, 0);
            str_writeln("cmd1 done", STDOUT);  // debug

            tee_pid = fork();
            if (tee_pid == 0) {
                // In tee_pid ...
                str_writeln("Start tee:", STDOUT);  // debug

                // TODO:
                // new pipe
                // connect stdout to NEW pipe_write.
                // close NEW pipe read
                // do tee on out_fd, reading on stdin w/read, 
                // and writing on stdout and the file using write.
                // close out_fd on end of data

                cmd2_pid = fork();
                if (cmd2_pid != 0) {
                    // In GchlidA ...
                    debugcmd("Start cmd2:", *cmd2);  // debug  

                    // TODO:
                    // Connect std in to NEW pipe_read
                    // close write end
                    // close out_fd
                    // do execvp
                    // If no more cmds, output to stdout, else to top_pipe_write          
                    exit(0);  // exit cmd2_pid

                } else {
                    // In tee (as cmd2's parent) ...
                    waitpid(cmd2_pid, NULL, 0);
                    str_writeln("tee done", STDOUT);  // debug
                    exit(0);  // exit tee_pid
                }

            } else {
                // In parent...
                waitpid(tee_pid, NULL, 0);
                str_writeln("Cmd 2 done.", STDOUT);

                close(top_pipe[0]);
                close(top_pipe[0]);
                // TODO: continue;
                return i;  // debug
            }
        }
    }
    return i;
}


// Main application driver
int main(int argc, char **argv) {
    command *commands;
    char *outfile_name;
    int cmd_count = 0;
    int out_fd;

    // Validate arg count
    if (argc < 8)
    {
        // str_write("ERROR: Too few arguments.", STDERR);
        // return -1;

        // debug
        argc = 9;
        argv[1] = "allfiles";
        argv[2] = "[";
        argv[3] = "ps";
        argv[4] = "aux";
        argv[5] = "]";
        argv[6] = "[";
        argv[7] = "grep";
        argv[8] = "dfast";
        argv[9] = "]";
        argv[10] = "[";
        argv[11] = "grep";
        argv[12] = "dfast";
        argv[13] = "]";
    }
    outfile_name = argv[1];

    // Parse cmd line args for commands to run, wrapping them in commands.
    commands = malloc(MAX_LEN * sizeof(command));
    cmd_count = get_cmds(argc, argv, commands);
    if (cmd_count < 2) {
        write_err("ERROR: Invalid argument format. Ensure matched braces.");
        free(commands);
        return -1;
    }

    // Open the output file
    out_fd = open(outfile_name, O_CREAT | O_WRONLY | O_TRUNC, 00644);
    if (out_fd < 0) {
        write_err("ERROR: Failed to open output file. Ensure proper format.");
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

    return 0;
}


// Prints the given char array to stderr with usage info and trailing newline.
void write_err(char *arr) {
    str_writeln(arr, STDERR);
    str_write("Usage: ", STDERR);
    str_writeln(USAGE, STDERR);
}


// Recursively parses array of the form: { '[', 'EXEC', 'ARGS', ']', '[', ... }
// Sets arr_out to inner contents of first set of matching outter braces,
// starting with arr_in[i]. Var in_len denotes length(arr_in).
// arr_out should have an initial dynamic mem allocation of 0.
// Vars j and k should initially be 0.
// Returns index of the first unread byte in arr_in, or -1 if parse error.
int parse_cmds(char **arr_in, char *arr_out, int in_len, int i, int j, int k) {
    // i = curr index of arr_in
    // j = curr index of arr_out
    // k = bracket level
    if (*arr_in[i] == '[') {
        k++;
    }
    else if (*arr_in[i] == ']') {
        k--;
    } else {
        int len = str_len(arr_in[i]);
        arr_out = realloc(arr_out, j + len);
        str_cpy(arr_in[i], &arr_out[j], len);
        str_cpy(" ", &arr_out[j + len], 1);
        j = j + len + 1;
    }
    i++;

    // On successful bracket match
    if (k < 1)
        return i;

    // On end of arr_in with no matched closing bracket
    if (i > in_len + 1 && k > 0)
        return -1;

    // Else, recurse
    parse_cmds(arr_in, arr_out, in_len, i, j, k);
}


// Starts the recursive CMD parse from the given command line argument vars.
// Results in cmds populated with structs of type command.
// Returns -1 on parse fail, else returns the number of commands in cmds.
int get_cmds(int argc, char **argv, command *cmds) {
    char *inner;                // I.e. "ls -lat" out of "[ ls -lat ]"
    char *cmd_str;              // I.e. "ls"
    char *args_str;             // I.e. "-lat"
    int i = 2;                  // As in argv[i]. Note, starts at 3rd arg.
    int results = 0;            // Return val. Success (>=0), or fail (-1).

    // Process an arbitrary number of CMD args.
    // Expected form is: { '[', 'EXECUTABLE', 'ARGS', ']', '[', ... , ']' }
    while (i < argc && results > -1) {
        // Parse for the "inner" portion of CMD arg
        inner = malloc(0);
        i = parse_cmds(argv, inner, argc, i, 0, 0);

        // On parse fail
        if (i == -1) {
            results = -1;

        // On parse success
        } else {
            // Split on executable/args
            cmd_str = malloc(0);
            args_str = malloc(0);
            split_str(inner, cmd_str, args_str, " ");

            // Instantiate a new command and push ref to cmds
            command cmd;
            str_cpy(cmd_str, cmd.cmd, str_len(cmd_str));
            str_cpy(args_str, cmd.args, str_len(args_str));
            cmds[results] = cmd;
            results++;
        }

        free(cmd_str);
        free(args_str);
    }
    
    free(inner);
    
    return results;
}

