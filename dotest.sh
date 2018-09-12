# valgrind --tool=memcheck ./a.out allfiles [ ps ] [ wc -w ]
#
# Should Fail:
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ [ ls -lat ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] [ [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ]];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ] ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo Unmatched bracket: ] ] [ wc -w ];
#
# Should Pass:
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ps aux ] [ grep SCREEN ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo Hello world ] [ wc -w ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo Hello world ] [ wc -w ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ cat pipeobserver.c pipeobserver.c ] [ wc -l ]
gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo These are matched brackets: [ ] ] [ wc -w ];