#undef SELF_PHOTO_VOTE
#ifdef SELF_PHOTO_VOTE
enum {
	SP_GG,
	SP_MM,
	SP_BABY,
	SP_LOVER,
	SP_GROUP,
	SP_ACTION,
	SP_FACE,
	SP_SKIRT,
	SP_NUM
};

static char self_an_vote_path[] =
    "0Announce/groups/GROUP_T/self_photo/M1085243614";

static int self_an_vote_type[5] = {
	1085028064,
	1085028072,
	1085028080,
	1085035260,
	1085035280
};

static char *self_an_vote_ctype[SP_NUM] = {
	"最佳时尚GG",
	"最佳时尚MM",
	"最佳情侣",
	"最佳团体",
	"最佳可爱baby",
	"最佳动作",
	"最佳表情",
	"最佳服饰"
};

static char *self_an_vote_limit[SP_NUM] = {
	"0",
	"1",
	"2",
	"3",
	"4",
	"0 1 3 4",
	"0 1 4",
	"0 1"
};
#endif
