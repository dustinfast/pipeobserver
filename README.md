# Pipe Observer

This application is a replacement for the GNU "tee" utility. Implemented 
using POSIX syscalls only, it pipes the output of COMMAND1 to the input of
COMMAND2, writing a copy of the piped data to the file named by OUTFILE.

## Usage

`$ ./pipeobserver OUTFILE [ COMMAND1 ARGS1 ] [ COMMAND2 ARGS2 ]`

For example:  
`$ ./pipeobserver outfile.dat [ ps ] [ grep pts ]`  
`$ ./pipeobserver outfile.dat [ ps aux ] [ wc ]`  
`$ ./pipobserver outfile.dat [ echo [ Hello World ] ] [ grep Hello ]`

Note: All square-brackets must be well-formed. A well-formed bracket is surrounded by whitespace and has a matching well-formed open/close bracket. Any brackets contained in ARGS are passed to COMMAND as arguments, however, they must also be well-formed. For example, the following case contains a malformed bracket in the first COMMAND block and will generate an error:  
`./pipeobserver outfile.dat [ echo Unmatched bracket: ] ] [ wc -w ]`

## Scalability

The original application specification did not require handling of more than two commands. However, the implementation was made with scalability in mind, therefore pipeobserver will handle an arbitrary number of commands with the following modifications to pipeobserver.c:

1. Adjust MAX_CMDS for the maximum number of commands desired.
2. Encapsulate fork_and_pipe()'s forking code-block inside a `for (int i = 0; i < cmd_count; i++)` block.
3. Redirect GrandChildB stdout to the top-level pipe input if there are more commands to be processed. If GrandChildB's stdout is redirected in this way, the next time Child1 is instantiated it's stdin is redirected to top-level pipe's output.

## Test Cases
This application was tested successfully on Debian 9 with the following use-cases:

``` bash
# Well-formed commands, passing
$ ./pipeobserver outfile.dat [ ps aux ] [ grep SCREEN ]
$ ./pipeobserver outfile.dat [ ls -lat ] [ wc -l ]
$ ./pipeobserver outfile.dat [ echo Hello world ] [ wc -w ]
$ ./pipeobserver outfile.dat [ echo Hello world ] [ wc -w ]
$ ./pipeobserver outfile.dat [ cat pipeobserver.c pipeobserver.c ] [ wc -l ]
$ ./pipeobserver outfile.dat [ echo These are matched brackets: [ ] ] [ wc -w ]

# Malformed commands, failing gracefully
$ ./pipeobserver
$ ./pipeobserver outfile.dat
$ ./pipeobserver outfile.dat [
$ ./pipeobserver outfile.dat [ ] [ ]
$ ./pipeobserver outfile.dat [ ls -lat ]

$ ./pipeobserver outfile.dat [ls -lat]       [ wc -l ]
$ ./pipeobserver outfile.dat [ ls -lat ]     [wc -l]
  
$ ./pipeobserver outfile.dat [ [ ls -lat ]     [ wc -l ]
$ ./pipeobserver outfile.dat [ ls -lat ]     [ [ wc -l ]
 
$ ./pipeobserver outfile.dat [ ls -lat ] ]   [ wc -l ]
$ ./pipeobserver outfile.dat [ ls -lat ]     [ wc -l ] ]

$ ./pipeobserver outfile.dat [ [ ls -lat ]     [ wc -l ] ]
$ ./pipeobserver outfile.dat [ [ ls -lat ] ] [ [ wc -l ] ]
$ ./pipeobserver outfile.dat [ [ ls -lat ] [ ] [ wc -l ] ]

$ ./pipeobserver outfile.dat [ echo Unmatched bracket: ] ] [ wc -w ]
$ ./pipeobserver outfile.dat [ echo Malformed brackets: [] ] [ wc -w ]

```

__Author:__ Dustin Fast, 2018
