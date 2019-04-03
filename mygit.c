#include <stdio.h>
#include "sqlite3.h"
#include<string.h>
#include<stdlib.h>
#include"mygit.h"
#include<dirent.h>
#include"sha1.h"
#include<assert.h>

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

static int exec_sql(const char*);

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

  /*index实际上是只有一份的，这边更加类似于一个配置*/
  char *sql_create_FileIndex = "CREATE TABLE IF NOT EXISTS FileIndex(" \
                             "id INT PRIMARY KEY," \
                             "file CHAR(41)," \
                             "FOREIGN KEY(file) REFERENCES File(sha1));";

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
static int exec_sql(const char *sql_s){

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
  g_index.fl = NULL;
  g_status.status = 1;//init
}

void GStatus(){
  if(!g_status.status){
    printf("not a mygit respo\n");
    return;
  }
}

static void indexAddfile(const char* name,const char* sha1){

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
  strcpy(fl->sha1,sha1);
  fl->next = NULL;
  FileList **p = &g_index.fl;
  while(*p) p = &(*p)->next;
  *p = fl;
  g_index.file_num++;
}

/*TODO:可能需要一个守护进程保存必要的信息避免频繁访问db*/
static void GetIndexFromDB(){

  const char *sql_get_index = "SELECT File.sha1,File.name From FileIndex " \
                               "JOIN FileIndex ON FileIndex.file = File.sha1";
  exec_sql(sql_get_index);
}

/*TODO:这些查询语句需要包装*,这里的代码完全只是测试*/
static void index2DB(){

  FileList *p = g_index.fl;
#define MAX_INSERT_FILE_NUM 100
/*TODO:这个值应该是可以在其他地方获得多平台定义值的*/
#define MAX_FILE_INSERT_LENGTH 256
  printf("num:%d\n",g_index.file_num);

  char* sql_insert = malloc(MAX_FILE_INSERT_LENGTH*MAX_INSERT_FILE_NUM);
  char* sql_insert_single = malloc(MAX_FILE_INSERT_LENGTH);
  if(!sql_insert || !sql_insert_single){
    perror(NULL);
    exit(-1);
  }
  strcpy(sql_insert,"INSERT INTO File(sha1,name) VALUES ");

  for(int i = 0;i < g_index.file_num;++i){
    assert(p);
    if(i != g_index.file_num-1)
      sprintf(sql_insert_single,"(\"%s\",\"%s\"),",p->sha1,p->name);
    else
      sprintf(sql_insert_single,"(\"%s\",\"%s\");",p->sha1,p->name);
    strcat(sql_insert,sql_insert_single);
    p = p->next;
  }
  printf("%s\n",sql_insert);
  exec_sql(sql_insert);
#if 0
  while(p){
    assert(p->name);
    printf("name:%s,sha1:%s\n",p->name,p->sha1);
    p = p->next;
  }
#endif
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
    while(plist && j++ != g_index.file_num){
      assert(plist->name);
      if(strcmp(*(file+i),plist->name) == 0)
        break;
      plist = plist->next;
    }

    if(j == g_index.file_num){
      /*index not finded the file,add*/
      indexAddfile(*(file+i),file_sha1);
    }else{
      /*index finded the file*/
      if(strcmp(plist->sha1,file_sha1) == 0){
        printf("file:%s exists\n",*(file+i));
        continue;
      }else{
        indexAddfile(*(file+i),file_sha1);
        /*add file*/
      }
    }
  }
  index2DB();
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
