
struct HZS {
   char p[2];
   char h[120][2];
   short f[120]; //һ�����֣�һ����Ƶ
};

struct WORDS {
   char p[100][2];
   char h[100][4];
   short f[100];  //������ĸ���������֣�һ����Ƶ
};

struct LWORDS {
   char p[100][6+6];
   char h[100][12];
   short f[100];
};

struct PINYINARRAY {
   short i[24]; //��s����index, ������ĸ
   struct HZS s[406];  //406��ƴ��
   struct WORDS w[24][24]; //23����ĸ���һ������ĸ
   struct LWORDS l[100];  //24*24*24����ĸ���ӳ�䵽1��99 s1*s2*s3%100
};
