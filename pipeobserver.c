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

#include "dstring.h"  // Custom string function lib

#define MAX_LEN (8192)

#define STDIN (STDIN_FILENO)
#define STDOUT (STDOUT_FILENO)
#define STDERR (STDERR_FILENO)

#define USAGE ("./pipeobserver OUTFILE [ CMD1 ] [ CMD2 ]")


// Pretty prints the given char arr to stderr with usage info.
void write_err(char *arr) 
{
    str_writeln(arr, STDERR);
    str_write("Usage: ", STDERR);
    str_writeln(USAGE, STDERR);
}


// Recursively parses a 2D array of the form: '[', 'CMD1', 'ARGS', ']', '[', ... 
// and sets arr_out to inner contents of first set of matching outter braces,
// starting at index i of arr_in. Var in_len denotes lenght of arr_in.
// arr_out should have an initial dynamic mem allocation of 0. Vars j and k
// should initially be 0.
// Returns -1 on fail, else the index of the first unread byte in arr_in.
int parse_cmds(char **arr_in, char *arr_out, int in_len, int i, int j, int k)
{
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


// Starts recursive parsing of CMD args from the command line arguments
int do_parse_cmds(int argc, char **argv) 
{
    char *inner;        // I.e. "ls -lat" out of "[ ls -lat ]"
    char *cmd;          // I.e. "ls"
    char *args;         // I.e. "-lat"
    int i = 2;          // As in argv[i]. Note, starts at 3rd arg.
    int result = 0;     // Return val. Success (0), or fail (-1).

    // Process each CMD arg
    while (i < argc && result == 0) {
        int result = 0;
        inner = malloc(0);
        cmd   = malloc(0);
        args  = malloc(0);

        i = parse_cmds(argv, inner, argc, i, 0, 0);

        // On success
        if (i != -1) {

        // On fail
        } else {
            write_err("ERROR: Malformed CMD argument.");
            str_writeln(inner, STDERR);  // debug
            result = -1;
        }

        // debug
        // printf("\n");
        // printf("OK: ");
        // printf(inner);
        // printf("\n");

        free(inner);
        free(cmd);
        free(args);
    }
    
    printf("\nDone\n");

    return result;
}

// Main application driver
int main(int argc, char **argv) 
{
    char *outfile;
    int outfile_fd;

    // Validate cmd line arg length
    if (argc < 8)
    {
        // str_write("ERROR: Too few arguments.", STDERR);
        
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
    outfile = argv[1];

    // Validate and open outfile
    if (str_len(outfile) <= 0) {
        str_write("ERROR: Output file length cannot be zero.\n", STDERR);
        return -1;
    }

    outfile_fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 00644);
    if (outfile_fd < 0) {
        str_write("ERROR: Failed to open OUTFILE.\n", STDERR);
        return -1;
    }

    // Parse and execute CMD args
    do_parse_cmds(argc, argv);

    // cleanup
    close(outfile_fd);

    return 0;
}