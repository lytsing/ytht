#ifndef __REGFORM_H
#define __REGFORM_H
/* regform.c */
struct REGINFO {
    char userid[100];           /* �ʺ� ID  */
    char realname[100];         /* ��ʵ���� */
    char career[100];           /* ����λ */
    char addr[100];             /* Ŀǰסַ */
};

int checkreg(struct REGINFO *reginfo, char err[100]);
#define NEWREGFILE MY_BBS_HOME "/new_register"
#define GETREGFILE MY_BBS_HOME "/new_register_getting"
#define SCANREGDIR MY_BBS_HOME "/scanregister_tmp/"
int getregforms(char *filename, int num, const char *userid);
#define EMAILCHECK "etc/mailcheck"
#define USEDEMAIL "etc/usedemail"
#define UNTRUSTEMAIL "etc/untrustemail"
#define TRUSTEMAIL "etc/trustemail"
#define INVITATIONDIR "invitation"
#define INVITATION "etc/invitation"
int trustEmail(char *email);
int do_send_emailcheck(const struct userec *tmpu, const struct userdata *udata, const char *randstr);
int send_emailcheck(const struct userec *tmpu, const struct userdata *udata);
int sendInvitation(const char *inviter, const char *toemail, const char *toname, const char *tmpfn);
#endif
