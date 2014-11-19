extern int convcode;
extern void redoscr();

int
switch_code()
{
	convcode = !convcode;
	redoscr();
	return convcode;
}
