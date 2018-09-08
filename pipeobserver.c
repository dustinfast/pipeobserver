// An application that pipes one processes stdout to the stdin of another, 
// and writing a copy of the data piped to the file given by outfile.
// Usage: ./pipeobserver outfile [ process1 ] [ process2 ]
// Example: ./pipeobserver outfile [ ps aux ] [ process2 ]
// Example: ./pipobserver outfile [ echo [ Hello ] [ wc -l ]
//
// Exit conditions:  0 = Success
//                  -1 = cannot open executable
//                  -2 = unmatched bracket(s)
//                  -3 
//
// Author: Dustin Fast, dustin.fast@hotmail.com, 2018

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXARGS (8192)
#define ARR_SIZE (19)


// Writes the given char array (cbuf) to the given file descriptor (fd).
// If fd == 0, writes to stdout instead.
int write_str(char *cbuf, int fd) {
    size_t char_count = 0;          // Num chars/bytes in cbuf
    size_t this_write = 0;      // Results of write()
    size_t total_written = 0;   // Total bytes of cbuf written

    // Count size of cbuf. Note use of ptr arithmetic.
    for (char *c=cbuf; *c!='\0'; c++)
        char_count++;

    // If empty string, do nothing
    if (!char_count) 
        return 0;

    // If not fd, fd = stdout
    if (!fd)
        fd = STDOUT_FILENO;

    // Write *cbuf to the given file descriptor, again note ptr arithmetic.
    while (total_written < char_count) {
        this_write = write(fd, cbuf + total_written, char_count - total_written);
        
        if (this_write < 0)
             return this_write; // on error

        total_written += this_write;
    }
}


// Does integer to string conversion, resulting in cbuf pointing to the string.
void itoa(int n, char *cbuf, int buf_len) {
    int cbuf_idx = 0;
    int dec_val;

    // Denote if neg number and convert to positive
    if (n < 0) {
        cbuf[cbuf_idx] = '-';
        n *= -1;
        cbuf_idx++;
    }

    // Append the cbuf_idx'th decimal place value to cbuf
    do {
        dec_val = n % 10;
        n = n / 10;
        cbuf[cbuf_idx] = '0' +  dec_val;  // ASCII conversion
        cbuf_idx++;
    } while (n > 0 && cbuf_idx < buf_len - 1);
    cbuf[cbuf_idx] = '\0';
}

// Main application driver
int main(int argc, char **argv) {
    char outfile[ARR_SIZE];

    if (argc < 8) {
        // write_str("Not enough arguments. Try: ", 0);
        // write_str("pipeobserver OUTFILE '[ PROCESS ARGS ] [ PROCESS ARGS ]'\n", 0);
        argc = 8;
        argv[1] = "allfiles";
        argv[2] = "[";
        argv[3] = "ls";
        argv[4] = "]";
        argv[5] = "[";
        argv[6] = "wc";
        argv[7] = "]";
    }

    // Output cmd line args, for debug
    char str[ARR_SIZE]; 
    for (int i = 0; i < argc; i++) {
        write_str("Arg #", 0);
        itoa(i, str, ARR_SIZE);
        write_str(str, 0);
        write_str(": ", 0);
        write_str(argv[i], 0);
        write_str("\n", 0);
    }

    // Recursively parse the cmd line args

    // Parse executable i.e. " [ ] "
    // A: Ensure executable w/'open'

    // B: Get process pipe w/'pipe'

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
    // write_str(fd, "test");
    // close(fd);

    return 0;
}