# gcc pipeobserver.c; ./a.out allfiles [ ps aux ] [ grep SCREEN ];
# gcc pipeobserver.c; ./a.out allfiles [ ls -lat ] ] [ wc -l ];
# gcc pipeobserver.c; ./a.out allfiles [ [ ls -lat ] [ wc -l ];
# gcc pipeobserver.c; ./a.out allfiles [ echo Hello world ] [ wc -w ];
gcc pipeobserver.c; ./a.out allfiles [ [ ls -lat ] [ wc -l ];