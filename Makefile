#TODO：需要重写，单独编译出中间文件，避免重复编译
all:
	gcc -o mygit mygit.c sqlite/sqlite3.c  sha1/sha1.c sha1/ubc_check.c -I./sqlite -I./sha1
