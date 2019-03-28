#TODO：需要重写，单独编译出中间文件，避免重复编译
all:
	gcc sqlite/sqlite3.c mygit.c -I./sqlite
