/***********************************************

QQWry.Dat ==> ip_arrange.txt ת������
 �ο� mok@smth�Ĵ��� ��telnet��ʵ������QQwry.dat����ip��ѯ��

***********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BYTE3INT(X)  (    ( X[0] & 0x000000FF ) \
			| ( ( X[1] & 0x000000FF ) <<  8 ) \
			| ( ( X[2] & 0x000000FF ) << 16 )  )

#define BYTE4INT(X)  (    ( X[0] & 0x000000FF ) \
			| ( ( X[1] & 0x000000FF ) <<  8 ) \
			| ( ( X[2] & 0x000000FF ) << 16 ) \
			| ( ( X[3] & 0x000000FF ) << 24 )  )

struct INDEXITEM {
	unsigned char ip[4];
	unsigned char offset[3];
};

void getData(unsigned char* str, FILE* fp, int max)
{
	int i = 0;
	while ( (*(str+i)=fgetc(fp)) && (i<(max-2)) )
		i++;
	str[i] = 0;
}
 int main()
{
	FILE *fp,*fpout;
	unsigned headpos,tailpos;
	int pos,curpos,offset,amount;
	unsigned char buf[80],content[200];
	struct INDEXITEM tmp;
	fp=fopen("QQWry.Dat","r");
	if(!fp){
		printf("cannot open file QQWry.Dat!");
		exit(0);
	}
	fpout=fopen("ip_arrange.txt","w");
	if(!fp){
		printf("cannot open file ip_arrange.txt!");
		exit(0);
	}
	fread(&headpos,sizeof(headpos),1,fp);
	fread(&tailpos,sizeof(tailpos),1,fp);
	amount = (tailpos - headpos)/sizeof(struct INDEXITEM);
//	printf("����%d����¼.\n",amount+1);
	for(offset=0;offset<=amount;offset++){
		fseek(fp,headpos+offset*sizeof(tmp),SEEK_SET);
		fread(&tmp,sizeof(tmp),1,fp);
		//ip��ַ�ο�ʼ
		fprintf(fpout,"%03u.%03u.%03u.%03u ",tmp.ip[3],tmp.ip[2],tmp.ip[1],tmp.ip[0]);
		pos = BYTE3INT(tmp.offset);
		fseek(fp, pos, SEEK_SET);
		fread(buf, 4, 1, fp);
		//ip��ַ�ν���
		fprintf(fpout, "%03u.%03u.%03u.%03u ", buf[3], buf[2], buf[1],
		buf[0]);
		fread(buf, 1, 1, fp);
		if ( buf[0] == 0x01 ) { // ���ҵ������ظ�, ��ת���µ�ַ
			fread(buf, 3, 1, fp);
			pos = BYTE3INT(buf);
			fseek(fp, pos, SEEK_SET);
			fread(buf, 1, 1, fp);
		}
		curpos=0;

		// ��ȡ����
		if ( buf[0] == 0x02 ) {
			// ��ȡ����ƫ��
			fread(buf, 3, 1, fp);
			// ���������Ϣ
			curpos = ftell(fp);
			pos = BYTE3INT(buf);
			fseek(fp, pos, SEEK_SET);
			fread(buf, 1, 1, fp);
		}
		if ( buf[0] == 0x01 || buf[0] == 0x02 ) {
			printf("There is something wrong....\n");
			return  -1;
		}

		if ( buf[0] )
			getData(buf+1, fp, 60);
		sprintf(content, "%s",strcmp(buf," CZ88.NET")?buf:"δ֪��ַ");

		// ��ȡ����
		if ( curpos )
			fseek(fp, curpos, SEEK_SET);
		fread(buf, 1, 1, fp);
		while ( buf[0] == 0x02 ) {
			// ��ȡ����ƫ��
			fread(buf, 3, 1, fp);
			pos = BYTE3INT(buf);
			fseek(fp, pos, SEEK_SET);
			fread(buf, 1, 1, fp);
		}
		if ( buf[0] == 0x01 || buf[0] == 0x02 ) {
			printf("There is something wrong....\n");
			return -1;
		}
		if ( buf[0] )
			getData(buf+1, fp, 80);
		//ȥ�����CZ88.NET
		if(strcmp(buf," CZ88.NET"))
			strcat(content,buf);
		//else
		//	strcat(content,"δȷ���ĵ�ַ");
		//ip��λ��
		fprintf(fpout,"%s\n",content);
	}
	fclose(fp);
	fclose(fpout);
	return 0;
}
