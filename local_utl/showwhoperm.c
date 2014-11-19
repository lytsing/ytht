#include "../include/bbs.h"
int
main(int argc, char *argv[])
{
	int fd1;
	struct userec rec;
	int size1 = sizeof (rec);

	if ((fd1 = open(PASSFILE, O_RDONLY, 0660)) == -1) {
		perror("open PASSWDFILE");
		return -1;
	}

	while (read(fd1, &rec, size1) == size1)
		if (		/*rec.userlevel&PERM_CLOAK||
				   rec.userlevel&PERM_XEMPT||
				   rec.userlevel&PERM_ACCOUNTS||
				   rec.userlevel&PERM_WELCOME||
				   rec.userlevel&PERM_OVOTE||
				   rec.userlevel&PERM_CHATCLOAK||
				   rec.userlevel&PERM_SYSOP ||
				   rec.userlevel&PERM_POSTMASK||
				   rec.userlevel&PERM_ANNOUNCE||
				   rec.userlevel&PERM_OBOARDS||
				   rec.userlevel&PERM_ACBOARD||
				   rec.userlevel&PERM_FORCEPAGE||
				   rec.userlevel&PERM_EXT_IDLE||
				   rec.userlevel&PERM_SPECIAL1||
				   rec.userlevel&PERM_SPECIAL2||
				   rec.userlevel&PERM_SPECIAL3||
				   rec.userlevel&PERM_SPECIAL4||
				   rec.userlevel&PERM_SPECIAL5||
				   rec.userlevel&PERM_SPECIAL6||
				   rec.userlevel&PERM_SPECIAL2 */
			   rec.userlevel & PERM_BASIC &&
			   rec.userid[0] &&
			   rec.stay >= 24 * 3600 &&
			   time(NULL) - rec.firstlogin >= 30 * 24 * 3600)
			printf("%s\n", rec.userid);
	close(fd1);
	return 0;
}
