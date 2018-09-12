# valgrind --tool=memcheck ./a.out allfiles [ ps ] [ wc -w ]
#
# Should Fail:
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ [ ls -lat ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] [ [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ]];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] ] [ wc -l ] ];
#
# Should Pass:
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ps aux ] [ grep SCREEN ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ ls -lat ] [ wc -l ];
# gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo Hello world ] [ wc -w ];
gcc -O3 -o pipeobserver pipeobserver.c; ./pipeobserver allfiles [ echo Hello world ] [ wc -w ];