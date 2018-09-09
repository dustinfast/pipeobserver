// An application that pipes one processes stdout to the stdin of another, 
// and writing a copy of the data piped to the file given by outfile.
// Usage: ./pipeobserver outfile [ process1 ] [ process2 ]
// Example: ./pipeobserver outfile [ ps aux ] [ process2 ]
// Example: ./pipobserver outfile [ echo [ Hello ] ] [ wc -l ]
// NOTE: Unmatched brackets prohibited in cmd line args.
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

struct command {
    char cmd[MAX_LEN];
    char args[MAX_LEN];
};
    
// TODO: Func declarations


// Prints the given char array to stderr with usage info and trailing newline.
void write_err(char *arr) {
    str_writeln(arr, STDERR);
    str_write("Usage: ", STDERR);
    str_writeln(USAGE, STDERR);
}


// Recursively parses array of the form: { '[', 'EXEC', 'ARGS', ']', '[', ... }
// Sets arr_out to inner contents of first set of matching outter braces
// beginning at arr_in[i]. Var in_len denotes length(arr_in).
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


    // On successful bracket match with inner content
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
int get_cmds(int argc, char **argv, struct command *cmds) {
    char *inner;                // I.e. "ls -lat" out of "[ ls -lat ]"
    char *cmd;                  // I.e. "ls"
    char *args;                 // I.e. "-lat"
    int i = 2;                  // As in argv[i]. Note, starts at 3rd arg.
    int result = 0;             // Return val. Success (>=0), or fail (-1).

    // Process an arbitrary number of CMD args.
    // Expected form is: { '[', 'EXECUTABLE', 'ARGS', ']', '[', ... , ']' }
    while (i < argc && result >= 0) {
        // Get the "inner" portion of CMD arg
        inner = malloc(0);
        i = parse_cmds(argv, inner, argc, i, 0, 0);

        // On parse fail
        if (i == -1) {
            result = -1;

        // On parse success
        } else {
            // Split CMD on executable/args and store resulting cmd in cmds
            cmd = malloc(0);
            args = malloc(0);
            split_str(inner, cmd, args, " ");

            str_writeln("Split:", STDOUT);
            str_writeln(cmd, STDOUT);
            str_writeln(args, STDOUT);

            str_cpy(cmd, cmds->cmd, str_len(cmd));
            str_cpy(args, cmds->args, str_len(args));
            cmds++;
            result++;

            str_writeln("Set:", STDOUT);
            str_writeln(cmds->cmd, STDOUT);
            str_writeln(cmds->args, STDOUT);
        }
    }

    free(inner);
    free(cmd);
    free(args);


    printf("\nDone\n");

    return result;
}


// Main application driver
int main(int argc, char **argv) {
    int outfile;
    struct command *commands;

    // int p=(int*)calloc(2, sizeof(struct command));

    // Validate cmd line arg count
    if (argc < 8)
    {
        // str_write("ERROR: Too few arguments.", STDERR);
        // return -1;

        // debug
        argc = 8;
        argv[1] = "allfiles";
        argv[2] = "[";
        argv[3] = "ps";
        argv[4] = "aux";
        argv[5] = "]";
        argv[6] = "[";
        argv[7] = "grep";
        argv[8] = "dfast";
        argv[9] = "]";
    }


    // Parse cmd line args for commands to run
    commands = malloc(0);
    if (get_cmds(argc, argv, commands) == -1) {
        write_err("ERROR: Invalid argument format. Ensure matched braces.");
        free(commands);
        return -1;
    }

    // Open outfile (denoted by argv[1])
    outfile = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 00644);
    if (outfile < 0) {
        write_err("ERROR: Failed to open output file.");
        free(commands);
        return -1;
    }

    close(outfile);
    free(commands);

    return 0;
}


    // // Open pipe. Note: pipe[0] = read end, pipe[1] = write end
    // int pipe_fds[2];
    // if (pipe(pipe_fds) != 0) {
    //     str_write("ERROR: Could not open pipe.", STDERR);
    //     perror(argv[0]);
    //     // TODO: Cleanup
    //     return -1;
    // }

    // // Spawn child proc 1
    // pid_t child1_pid = fork();
    // char arr[BUF_SIZE];
    // if (child1_pid == 0) {
    //     // Child 1...
    //     printf("In child1 . Pid is %d.\n", (int) getpid());

    //     split_str(cmd1, curr_cmd, curr_args, " ");
    //     str_write("Running ", STDOUT);
    //     str_write(curr_cmd, STDOUT);
    //     str_write(" ", STDOUT);
    //     str_write(curr_args, STDOUT);
    //     str_write("\n", STDOUT);

    //     close(pipe_fds[0]);
    //     dup2(pipe_fds[1], STDOUT_FILENO);   // redirect stdout

    //     char* exec_args[] = {curr_cmd, curr_args, NULL};
    //     execvp(argv[0], exec_args);
    //     close(pipe_fds[1]);
    //     printf("Child1: Exiting.\n");
        
    //     free(curr_cmd);
    //     free(curr_args);
    //     exit(0);

    // } else {
    //     // Parent...
    //     printf("In parent1. Waiting for child %d", (int) child1_pid);
    //     printf("\n");
    //     waitpid(child1_pid, NULL, STDOUT);
        
    //     close(pipe_fds[1]);
        
    //     while (read(pipe_fds[0], arr, BUF_SIZE) > 0)  {
    //         // write_str(arr, pipe_fds[0]):
    //         // printf("%s", arr);
    //     }

    //     pid_t child2_pid = fork();
    //     if (child2_pid == 0) {
    //         printf("In child2, Pid is: %d\n", (int) getpid());
    //         printf("\n");

    //         split_str(cmd2, curr_cmd, curr_args, " ");
    //         str_write("Running ", STDOUT);
    //         str_write(curr_cmd, STDOUT);
    //         str_write(" ", STDOUT);
    //         str_write(curr_args, STDOUT);
    //         str_write("\n", STDOUT);
    //         printf("Child2: Exiting.\n");

    //         exit(0);
    //     } else {
    //         printf("In parent2 . Waiting for child %d", (int) child2_pid);
    //         waitpid(child2_pid, NULL, STDOUT);
    //         printf("\nParent2: Done\n");
    //     }
    //     printf("\nParent1: Done\n");
    // }

    // // close(outfile);
    // free(cmd1);
    // free(cmd2);
    // free(curr_cmd);
    // free(curr_args);
    // str_write("Done.\n", STDOUT);