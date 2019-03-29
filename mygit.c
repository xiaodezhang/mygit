#include <stdio.h>
#include "sqlite3.h"
#include<string.h>
#include<stdlib.h>
#include"mygit.h"
#include<dirent.h>
#include"sha1.h"

#define DBNAME "test.db"
Respo g_respo;
Index g_index;
Status g_status;

static int file2sha1(const char* fileName,char *sha1Buffer){

  FILE* fd;
  unsigned char hash2[20];
  char buffer[65536];
  size_t size;
  SHA1_CTX ctx2;
  int i,j,foundcollision;

  SHA1DCInit(&ctx2);

  fd = fopen(fileName,"rb");
  if(!fd){
    fprintf(stderr, "cannot open file: %s: %s\n", fileName, strerror(errno));
    return 1;
  }

  while(1){
    size = fread(buffer,1,65536,fd);
    SHA1DCUpdate(&ctx2,buffer,(unsigned)(size));
    if(size != 65536)
      break;
  }
  if(ferror(fd)){
    fprintf(stderr,"error while reading file: %s: %s\n",fileName,strerror(errno));
    fclose(fd);
    return 1;
  }
  if(!feof(fd)){
    fprintf(stderr,"not end of file?: %s: %s\n",fileName,strerror(errno));
    fclose(fd);
    return 1;
  }

  foundcollision = SHA1DCFinal(hash2,&ctx2);

  for(j = 0; j < 20;++j){
    sprintf(buffer+(j*2),"%02x",hash2[j]);
  }

  buffer[20*2] = 0;
  strcpy(sha1Buffer,buffer);
  fclose(fd);
  return 0;
}

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

void GHelp(){
  printf("help!help!");
}

void GInit(){

/*TODO:运行外部程序需要更多的内容*/
  system("mkdir .mygit");
  create_tables();
  g_index.file_num = 0;
  g_status.status = 1;//init
}

void GStatus(){
  if(!g_status.status){
    printf("not a mygit respo\n");
    return;
  }
}

static void indexAddfile(const char* name){

  FileList *fl = malloc(sizeof(*fl));
  if(!fl){
    perror(NULL);
    exit(-1);
  }
  fl->name = malloc(strlen(name)+1);
  if(!fl->name){
    perror(NULL);
    exit(-1);
  }
  strcpy(fl->name,name);
}

void GAdd(const char** file,int num){

  char file_sha1[41];
  for(int i = 0;i != num;++i){
    if(file2sha1(*(file+i),file_sha1) != 0){
      printf("[GAdd]file sha1 error\n");
      exit(-1);
    }
    FileList *plist = g_index.fl;
    int j= 0;
    while(plist || j++ != g_index.file_num){
      if(strcmp(file_sha1,plist->sha1) == 0)
        break;
    }

    if(j == g_index.file_num){
      /*index not finded the file,add*/
      indexAddfile(*(file+i));
    }else{
      /*index finded the file*/
      if(strcmp(plist->name,*(file+i)) == 0){
        printf("file:%s exists\n",*(file+i));
        continue;
      }else{
        indexAddfile(*(file+i));
        /*add file*/
      }

    }
  }
}



/*TODO:命令处理需要修改，但是我们先看看效果吧*/
int main(int argc, char **argv){

  if(argc < 2)
    GHelp();
  if(strcmp(argv[1],"init") == 0){
    GInit();
    return 0;
  }
  if(strcmp(argv[1],"status") == 0){
    GStatus();
    return 0;
  }
  if(strcmp(argv[1],"add") == 0){
    GAdd((const char**)argv+2,argc-2);
    return 0;
  }

  return 0;
}
