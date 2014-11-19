#ifdef ENABLE_MYSQL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "bbs.h"
#include "usesql.h"

// fix me 评价文章的人数
#define pingjiarenshu 2
// fix me 近期时间 秒为单位 比如近期为7天则如下
#define jinrishijian 7*24*3600
// fix me www 方式的精彩文件的文
#define fnamewww MY_BBS_HOME "/wwwtmp/navpart.txt"
// fix me telnet 方式10大精彩话题文
#define fnametelnet MY_BBS_HOME "/etc/posts/good10"

int main(void)
{
  MYSQL *mysql=NULL;
  MYSQL_ROW row;
  MYSQL_RES *res;
  FILE *fp1,*fpwww,*fptelnet;
  char sqlbuf[512],fname[200];
  char *start[5]={"★☆☆☆☆","★★☆☆☆","★★★☆☆",
		  "★★★★☆","★★★★★"};
  int qryflag,i=1;
  time_t curtime;
  struct fileheader *fh;
  curtime=time(NULL);

  fh=(struct fileheader *)malloc(sizeof(struct fileheader));
  mysql=mysql_init(mysql);

  mysql=mysql_real_connect(mysql,"localhost",SQLUSER, SQLPASSWD, SQLDB,0, NULL,0);
  if (!mysql) {
      exit(0);    
  }

  sprintf(sqlbuf,"select avg(class),count(filename) as cnt,board,mid(filename,3,10) as tt from articlevote group by filename having  cnt>=%d and %ld- tt<%d order by  cnt desc,tt desc " , pingjiarenshu,curtime,jinrishijian );
 
  qryflag=mysql_query(mysql,sqlbuf);
  res=mysql_store_result(mysql);
  if((fpwww=fopen( fnamewww,"w"))==NULL){
        printf("Cannot create file " fnamewww "\n");
	exit(0);
  }
  if((fptelnet=fopen( fnametelnet,"w"))==NULL){
        printf("Cannot create file " fnametelnet "\n");
	exit(0);
  }
  fprintf(fptelnet,"                \033[1;34m-----\033[37m=====\033[41;37m近日十大精彩话题回顾 \033[40m=====\033[34m-----\033[0;37m           \033[m\n\n");
  while((row = mysql_fetch_row(res))){
          int ff=0;
	  sprintf(fname,MY_BBS_HOME "/boards/%s/.DIR",row[2]);
       	  if((fp1=fopen(fname,"r"))==NULL){
	    	printf("Cannot open file %s\n",fname);
	    	exit(0);
 	  }
	  while(!feof(fp1)){
		fread( fh,1,sizeof(struct fileheader),fp1);
	        if(fh->filetime==atoi(row[3])){
			 ff=1;
		 	break;
		}
	 }		
        fprintf(fpwww,"%s %s %s %s %d %s\n",row[0],row[1],row[2],fh->owner,fh->thread,fh->title);
	if(i<=10)
		fprintf(fptelnet,"\033[1m第\033[31m%2d \033[37m名  信区 : \033[33m%-20s\033[37m【\033[32m%s\033[37m】\033[36m %3s \033[37m人\033[35m%14s \033[0;37m \033[0m\n\033[1;35m      \033\[37m标 题: \033[44;37m%-58s\033[m\n",
i,row[2],start[atoi(row[0])-1],row[1], fh->owner,fh->title);
	i++;
	fclose(fp1);
       	
       }
  fclose(fpwww);
  fclose(fptelnet);
  mysql_close(mysql);
  return 0;
}
#else
int
main()
{
	return 0;
}
#endif
