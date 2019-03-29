#ifndef MYGIT_H
#define MYGIT_H

typedef struct TagRespo{
  char* gitRes;
}Respo;

typedef struct TagStatus{
  char status;

}Status;

typedef struct TagFileList{
  char sha1[41];
  char *name;
  struct TagFileList* next;
}FileList;

/*used as staged respo,right?*/
typedef struct TagIndex{
  int file_num;
  /*这里使用链表应该是合适的*/
  FileList* fl;
}Index;

#endif
