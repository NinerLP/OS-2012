all: readlines-main unsed

all-shared: readlines-shared unsed-shared

readlines-main: readlines-main.o readlines.o
	gcc readlines-main.o readlines.o -o readlines-main
 
readlines-shared: readlines-main.o libreadlines
	gcc -L . readlines-main.o -lreadlines -o readlines-main
 
unsed: unsed.o readlines.o
	gcc unsed.o readlines.o -lpcre -o unsed
	
unsed-shared: unsed.o libreadlines
	gcc -L . unsed.o -lreadlines -lpcre -o unsed

unsed.o:
	gcc -c unsed.c
 
readlines-main.o:
	gcc -c readlines-main.c
       
readlines.o:
	gcc -c readlines.c
        
libreadlines: readlines-shared.o
	gcc -shared readlines-shared.o -o libreadlines.so
    
readlines-shared.o:
	gcc -c -fPIC readlines.c -o readlines-shared.o
     
             
clean:
	rm -rf *.o readlines-main libreadlines.so unsed
