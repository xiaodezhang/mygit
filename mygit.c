#include <stdio.h>
#include "sqlite3.h"
#include<string.h>
#include<stdlib.h>

#define DBNAME "test.db"
static int exec_sql(char*);

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

/*git中HEAD指向branch或者commit,这里是否需要修改*/
void *HEAD;

/*pragma table_inf(table_name); will show the table struct*/
void create_tables(){

  char* sql_s;
  /*TODO:name length?*/
  char *sql_create_File = "CREATE TABLE IF NOT EXISTS File(" \
                           "sha1 CHAR(41) PRIMARY KEY," \
                           "name VARCHAR(200));";  

  /*TODO:int大小是否会溢出，sqlite是否有自动处理*/
  /*commit 似乎是sqlite的一个关键字，不能使用*/
  char *sql_create_GCommit = "CREATE TABLE IF NOT EXISTS GCommit(" \
                             "id INT PRIMARY KEY," \
                             "pCommit INT);";

  char *sql_create_FileCommit = "CREATE TABLE IF NOT EXISTS FileCommit(" \
                             "id INT PRIMARY KEY," \
                             "file CHAR(41)," \
                             "gCommit INT," \
                             "FOREIGN KEY(file) REFERENCES File(sha1)," \
                             "FOREIGN KEY(gCommit) REFERENCES GCommit(id));";

  char *sql_create_Branch = "CREATE TABLE IF NOT EXISTS Branch(" \
                             "id INT PRIMARY KEY," \
                             "gCommit INT);";


  char *sql_create_Tag = "CREATE TABLE IF NOT EXISTS Tag(" \
                             "id INT PRIMARY KEY," \
                             "gCommit INT);";

  sql_s = malloc(strlen(sql_create_File)+strlen(sql_create_GCommit)+
      strlen(sql_create_FileCommit)+strlen(sql_create_Branch)+
      strlen(sql_create_Tag)+ 1);
  if(!sql_s){
    printf("malloc for create tables sql error\n");
    return;
  }
  sql_s[0] = '\0';
  strcat(sql_s,sql_create_File);
  strcat(sql_s,sql_create_GCommit);
  strcat(sql_s,sql_create_FileCommit);
  strcat(sql_s,sql_create_Branch);
  strcat(sql_s,sql_create_Tag);
  exec_sql(sql_s);
  free(sql_s);
}

/**********************************************************************
 * *sqlite 执行sql函数，包含连接，执行和关闭
 **********************************************************************
 *return:0 success
 *param:sql语句
***********************************************************************/
static int exec_sql(char *sql_s){

  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  rc = sqlite3_open(DBNAME, &db);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }
  rc = sqlite3_exec(db, sql_s,0, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
 
}
static void test_sqlite(){

        
}

int main(int argc, char **argv){

  create_tables();

  return 0;
}
