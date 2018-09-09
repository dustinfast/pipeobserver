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

#define MAX_ARGS (8192)
#define ARR_SIZE (64)
#define BUF_SIZE (1024)


// Given a null-terminated string, returns the length of that string.
size_t str_len(char* arr) {
    int length = 0;
    for (char *c=arr; *c!='\0'; c++)
        length++;

    return length;
}


// Writes the given char array to the given file descriptor (fd).
// If fd == 0, writes to stdout instead.
int str_write(char *arr, int fd) {
    size_t curr_write = 0;
    size_t total_written = 0;
    size_t char_count = str_len(arr);

    // If empty string, do nothing
    if (!char_count) 
        return 0;

    // If not fd, fd = stdout
    if (!fd)
        fd = STDOUT_FILENO;

    // Write *arr to the given file descriptor.
    while (total_written < char_count) {
        curr_write = write(fd, arr + total_written, char_count - total_written);
        
        if (curr_write < 0)
             return curr_write; // on error
        total_written += curr_write;
    }
}


// Does integer to string conversion, results in *arr_out == resuling_string.
void itoa(int n, char *arr_out, int arr_len) {
    int i = 0;      // arr_out index
    int dec_val;

    // Denote if neg number and convert to positive
    if (n < 0) {
        arr_out[i] = '-';
        n *= -1;
        i++;
    }

    // Convert i'th decimal place's value to ASCII and set arr_out[i] == ASCII
    do {
        dec_val = n % 10;
        n = n / 10;
        arr_out[i] = '0' +  dec_val;
        i++;
    } while (n > 0 && i < arr_len - 1);
    arr_out[i] = '\0';
}


// Copies the given number of bytes from arr_in to arr_out.
void str_cpy(char *arr_in, char *arr_out, int count) {
    for (int i = 0; i < count; i++) {
            arr_out[i] = arr_in[i];
    }
}


// Splits the string given by arr_in in two on the first occurance of seperator
void split_str(char *arr_in, char *arr_out1, char *arr_out2, char *seperator) {
    int len = str_len(arr_in);
    char *curr = arr_out1;
    int j = 0;

    for (int i=0; i<len; i++) {
        if (arr_in[i] == *seperator) {
            curr = arr_out2;
            j = 0;
            continue;
        }

        curr = realloc(curr, j + 1);
        str_cpy(&arr_in[i], &curr[j], 1);
        j++;
    }
}


// Recursively parses a 3D array of strings of the form "[", "CMD1", "ARGS", "]".
// ARGS may contain more matched brackets. Results in *arr_out = "CMD1 ARGS",
// and returns the index of the first unread byte in arr_in.
int parse_args(char **arr_in, char *arr_out, int i, int j, int level, int max_i)
{
    if (*arr_in[i] == '[') {
        level++;
    }
    else if (*arr_in[i] == ']') {
        level--;
    } else {
        int len = str_len(arr_in[i]);
        arr_out = realloc(arr_out, j + len);
        str_cpy(arr_in[i], &arr_out[j], len);
        str_cpy(" ", &arr_out[j + len], 1);
        j = j + len + 1;
    }
    i++;

    if (level < 1 || i > max_i)
        return i;

    parse_args(arr_in, arr_out, i, j, level, max_i);
}


// Main application driver
int main(int argc, char **argv) {
    char *outfile;
    char *cmd1 = malloc(0);
    char *cmd2 = malloc(0);
    char *curr_cmd = malloc(0);
    char *curr_args = malloc(0);
    

    str_write("Pipe Observer - Pipes CMD1 output to CMD2 input, ", 0);
    str_write("storing piped data in OUTFILE.\n", 0);
    str_write("Usage: ./pipeobserver OUTFILE [ CMD1 ] [ CMD2 ]\n\n", 0);

    // Validate cmd line arg length
    if (argc < 8)
    {
        // str_write("ERROR: Too few arguments.", 0);
        
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

    // debug
    // char str[ARR_SIZE];
    // for (int i = 0; i < argc; i++)
    // {
    //     str_write("Arg #", 0);
    //     itoa(i, str, ARR_SIZE);
    //     str_write(str, 0);
    //     str_write(": ", 0);
    //     str_write(argv[i], 0);
    //     str_write("\n", 0);
    // }

    // Validate outfile name
    if (str_len(outfile) <= 0)
    {
        str_write("ERROR: Output file length cannot be zero.\n", 0);
        return -1;
    }

    // Parse/validate commands
    int bracket_level = 0;
    int i = 2;      // argv index (note starting w/3rd arg)
    int j = 0;      // cmd index

    i = parse_args(argv, cmd1, i, 0, bracket_level, argc);
    parse_args(argv, cmd2, i, 0, bracket_level, argc);

    if (str_len(cmd1) == 0 || str_len(cmd2) == 0) {
        str_write("ERROR: Malformed CMD1 or CMD2 argument.", 0);
        // TODO: Cleanup
        return -1;
    }

    // Open pipe. Note: pipe[0] = read end, pipe[1] = write end
    int pipe_fds[2];

    if (pipe(pipe_fds) != 0) {
        str_write("ERROR: Could not open pipe.", 0);
        perror(argv[0]);
        // TODO: Cleanup
        return -1;
    }

    // Spawn child proc 1
    pid_t child1_pid = fork();
    char arr[BUF_SIZE];
    if (child1_pid == 0) {
        // Child 1...
        printf("In child1 . Pid is %d.\n", (int) getpid());

        split_str(cmd1, curr_cmd, curr_args, " ");
        str_write("Running ", 0);
        str_write(curr_cmd, 0);
        str_write(" ", 0);
        str_write(curr_args, 0);
        str_write("\n", 0);

        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);   // redirect stdout

        char* exec_args[] = {curr_cmd, curr_args, NULL};
        execvp(argv[0], exec_args);
        close(pipe_fds[1]);
        printf("Child1: Exiting.\n");
        // TODO: Cleanup?
        exit(0);
    
    // Parent
    } else {
        // Parent...
        printf("In parent1. Waiting for child %d", (int) child1_pid);
        waitpid(child1_pid, NULL, 0);
        
        close(pipe_fds[1]);
        
        while (read(pipe_fds[0], arr, BUF_SIZE) > 0)  {
            // write_str(arr, pipe_fds[0]):
            printf("%s", arr);
        }

        pid_t child2_pid = fork();
        if (child2_pid == 0) {
            printf("In child2, Pid is: %d\n", (int) getpid());
            split_str(cmd2, curr_cmd, curr_args, " ");
            str_write("Running ", 0);
            str_write(curr_cmd, 0);
            str_write(" ", 0);
            str_write(curr_args, 0);
            str_write("\n", 0);
            printf("Child2: Exiting.\n");

            exit(0);
        } else {
            printf("In parent2 . Waiting for child %d", (int) child2_pid);
            waitpid(child2_pid, NULL, 0);
            printf("\nParent2: Done\n");
        }
        printf("\nParent1: Done\n");
    }

    free(cmd1);
    free(cmd2);
    free(curr_cmd);
    free(curr_args);
    str_write("Done.\n", 0);

    // A: Ensure executable w/'open'


    // C: Create first child process of self. Child connects it's stdout to 
    //  the write end of pipe (inherited from parent), and closing the other end
    //  using dup2 and close
    // D: Child then replaces itself w/process corresponding w/first executable.
    //   to run using execvp, passing args appropriately

    // E: Create 2nd child process. 2nd child connects its stdin to read end of pipe
    // and closes the other end.
    // F: Child then creates 2nd pipe


    // File handler
    // int fd;
    // fd = open("foo.txt", O_CREAT | O_WRONLY | O_TRUNC, 00644);
    // str_write(fd, "test");
    // close(fd);

    

    return 0;
}