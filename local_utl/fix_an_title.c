#include <stdio.h>
#include <string.h>

char * gettitle(FILE *fp)
{
	static char buf[256];
	if(!fgets(buf,sizeof(buf),fp))
		return NULL;
	if(!fgets(buf,sizeof(buf),fp))
		return NULL;
	if(strncmp(buf,"Бъ  Ьт: ",8))
		return NULL;
	return (buf+8);
}
int main()
{
	FILE *fp, *tmp;
	char buf[256],name[256];
	char *ptr;
	fp=fopen(".Names","r");
	if(!fp)
		return -1;
	while(fgets(buf,sizeof(buf),fp)){
		if(strncmp(buf,"Name=M.",7)){
			printf("%s",buf);
			continue;
		}
		ptr=strchr(buf+7,'.');
		if(!ptr){
			printf("%s",buf);
			continue;
		}
		strcpy(name,buf+5);
		strtok(name,"\n");
		strtok(name," ");
		tmp=fopen(name,"r");
		if(!tmp){
			printf("%s",buf);
			continue;
		}
		if((ptr=gettitle(tmp))){
			fclose(tmp);
			printf("Name=%s",ptr);
			continue;
		}
		else{
			printf("%s",buf);
			continue;
		}

	}
	return 0;
}
