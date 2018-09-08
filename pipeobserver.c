#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXARGS (8192)
#define ARR_SIZE (19)

// Printf equivelant. Uses only system calls to write the given char
// buffer (cbuf) to the given file descriptor (fd).
// If fd == 0, writes to stdout instead.
int write_str(char *cbuf, int fd) {
    size_t ccount = 0;          // Num chars/bytes in cbuf
    size_t this_write = 0;      // Results of write()
    size_t total_written = 0;   // Total bytes of cbuf written
    char *c;

    // Count size of cbuf. Note use of ptr arithmetic.
    for (c=cbuf; *c!='\0'; c++)
        ccount++;

    // If empty string, do nothing
    if (!ccount) 
        return 0;

    // If not fd, fd = stdout
    if (!fd)
        fd = STDOUT_FILENO;

    // Write *cbuf to the given file descriptor, again note ptr arithmetic.
    while (total_written < ccount) {
        this_write = write(fd, cbuf + total_written, ccount - total_written);
        
        if (this_write < 0)
             return this_write; // on error

        total_written += this_write;
    }
}

// Sets cbuf to integer n's null-terminated string equivelant.
void itoa(int n, char *cbuf, int buf_len) {
    int i = 0;          // Current index of cbuf
    int dec_val;        // A decimal-place value

    // Denote if neg number and convert to positive
    if (n < 0) {
        cbuf[i] = '-';
        n *= -1;
        i++;
    }

    // Append the i'th decimal place value to cbuf
    do {
        dec_val = n % 10;
        n = n / 10;
        cbuf[i] = '0' +  dec_val;
        i++;
    } while (n > 0 && i < buf_len - 1);
    cbuf[i] = '\0';  // null terminate string
}


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
int main(int argc, char **argv) {
    char outfile[ARR_SIZE]

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

    // Parse cmd args, starting with argv[1]
    for (int i = 1; i < argc; i++) {
        switch argv[i]
    }

    return 0;
}