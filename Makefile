obj = mygit.o sqlite3.o sha1.o ubc_check.o

mygit:$(obj)
	gcc -o $@ $(obj) 
mygit.o:mygit.c
	gcc -c $^ -I./sqlite -I./sha1
sqlite3.o:sqlite/sqlite3.c
	gcc -o $@ -c $^
sha1.o:sha1/sha1.c
	gcc -o $@ -c $^
ubc_check.o:sha1/ubc_check.c
	gcc -o $@ -c $^
clean:
	rm $(obj) mygit
