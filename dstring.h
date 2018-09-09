// A collection of string handlers, implemented using only system calls.
// A string in this context is defined as a '\0'. terminated array of chars.
//
// Author: Dustin Fast, 2018


// Given a null-terminated string, returns the length of that string.
size_t str_len(char *arr) {
    int length = 0;
    for (char *c=arr; *c!='\0'; c++)
        length++;

    return length;
}

// Writes the given string to the given file descriptor, fd.
int str_write(char *arr, int fd) {
    size_t curr_write = 0;
    size_t total_written = 0;
    size_t char_count = str_len(arr);

    // If bad fd
    if (fd < 0) {
        str_write("Invalid file descriptor passed to str_write", STDERR_FILENO);
        return -1;
    }

    // If empty string , do nothing
    if (!char_count) 
        return 0;

    // Write string to the given file descriptor.
    while (total_written < char_count) {
        curr_write = write(fd, arr + total_written, char_count - total_written);
        
        if (curr_write < 0)
            return curr_write; // on error
        total_written += curr_write;
    }
}


// Writes the given string to the given file descriptor, followed by a newline.
int str_writeln(char *arr, int fd) {
    str_write(arr, fd);
    str_write("\n", fd);
}


// Prints the given interger to the given file descriptor.
// TODO: int str_iwrite(int n, int fd) {
//     int digit_count = ...
//     char buff[digit_count];
//     str_write(buff, STDOUT);
// }


// Does integer to string conversion, setting *arr_out to the resuling_string.
void itoa(int n, char *arr_out, int out_len) {
    int i = 0;
    int dec_val;

    // If negative number, write neg symbol and convert to positive
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
    } while (n > 0 && i < out_len - 1);
    arr_out[i] = '\0';
}


// Copies the given number of bytes from arr_in to arr_out.
// Assumes arr_out is of sufficient size.
void str_cpy(char *arr_in, char *arr_out, int count) {
    // TODO: Trim mem alloc
    for (int i = 0; i < count; i++) {
            arr_out[i] = arr_in[i];
    }
}


// Append given number of bytes from arr_in to arr_out, increasing size(arr_out)
// by that many bytes. Must also give current size of arr_out as out_len.
// Assumes: arr_out addrss space is reallocatable w/ realloc().
// Returns: The new length of arr_out.
int str_append(char *arr_in, char *arr_out, int out_len, int count) {
    arr_out = realloc(arr_out, out_len + count);
    int j = out_len - 1;

    for (int i = 0; i < count; i++) {
        arr_out[j] = arr_in[i];
        j++;
    }
}


// Splits the string given by arr_in in two on the first occurance of seperator.
// Assumes: arr_out1 and arr_out2 have a dynamically allocated size of 0.
// Result: arr_out1 = arr_in[:seperator], arr_out2 = string[seperator+1:]
// Note that the seperator is left out of the resulting arr_out1 and arr_out2.
// If no seperator found, arr_out1 = arr_in and arr_out2 is empty.
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
