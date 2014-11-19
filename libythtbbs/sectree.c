#include <stdio.h>
#include "sectree.h"
const struct sectree sectree;
static const struct sectree sectree_0;
static const struct sectree sectree_0_0;
static const struct sectree sectree_0_1;
static const struct sectree sectree_0_2;
static const struct sectree sectree_1;
static const struct sectree sectree_1_0;
static const struct sectree sectree_1_1;
static const struct sectree sectree_1_2;
static const struct sectree sectree_1_3;
static const struct sectree sectree_1_4;
static const struct sectree sectree_2;
static const struct sectree sectree_3;
static const struct sectree sectree_4;
static const struct sectree sectree_5;
static const struct sectree sectree_6;
static const struct sectree sectree_7;
static const struct sectree sectree_8;
static const struct sectree sectree_9;
static const struct sectree sectree_9_0;
static const struct sectree sectree_9_1;
static const struct sectree sectree_9_2;
static const struct sectree sectree_9_3;
static const struct sectree sectree_9_4;
static const struct sectree sectree_9_5;
static const struct sectree sectree_10;
static const struct sectree sectree_11;
static const struct sectree sectree_11_0;
static const struct sectree sectree_11_1;
static const struct sectree sectree_11_2;
static const struct sectree sectree_11_3;
static const struct sectree sectree_11_4;
static const struct sectree sectree_11_5;
static const struct sectree sectree_12;
static const struct sectree sectree_13;
static const struct sectree sectree_14;
static const struct sectree sectree_15;
static const struct sectree sectree_15_0;
static const struct sectree sectree_15_1;
static const struct sectree sectree_16;
/*------ sectree -------*/
const struct sectree sectree = {
	parent: NULL,
	title: "真水无香 BBS",
	basestr: "",
	seccodes: "0123456789TYXHLSC",
	introstr: "69742XYS|5T38HL10",
	des: "",
	nsubsec: 17,
	subsec: {
		&sectree_0,
		&sectree_1,
		&sectree_2,
		&sectree_3,
		&sectree_4,
		&sectree_5,
		&sectree_6,
		&sectree_7,
		&sectree_8,
		&sectree_9,
		&sectree_10,
		&sectree_11,
		&sectree_12,
		&sectree_13,
		&sectree_14,
		&sectree_15,
		&sectree_16,
	}
};
/*------ sectree_0 -------*/
static const struct sectree sectree_0 = {
	parent: &sectree,
	title: "BBS 系统",
	basestr: "0",
	seccodes: "QZX",
	introstr: "Q|BZX",
	des: "[本站]",
	nsubsec: 3,
	subsec: {
		&sectree_0_0,
		&sectree_0_1,
		&sectree_0_2,
	}
};
/*------ sectree_0_0 -------*/
static const struct sectree sectree_0_0 = {
	parent: &sectree_0,
	title: "区务管理",
	basestr: "0Q",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_0_1 -------*/
static const struct sectree sectree_0_1 = {
	parent: &sectree_0,
	title: "转信组管理",
	basestr: "0Z",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_0_2 -------*/
static const struct sectree sectree_0_2 = {
	parent: &sectree_0,
	title: "系统",
	basestr: "0X",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_1 -------*/
static const struct sectree sectree_1 = {
	parent: &sectree,
	title: "北京大学",
	basestr: "1",
	seccodes: "LWYST",
	introstr: "LW|YST",
	des: "[校园]",
	nsubsec: 5,
	subsec: {
		&sectree_1_0,
		&sectree_1_1,
		&sectree_1_2,
		&sectree_1_3,
		&sectree_1_4,
	}
};
/*------ sectree_1_0 -------*/
static const struct sectree sectree_1_0 = {
	parent: &sectree_1,
	title: "理工和综合院系",
	basestr: "1L",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_1_1 -------*/
static const struct sectree sectree_1_1 = {
	parent: &sectree_1,
	title: "人文社会院系",
	basestr: "1W",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_1_2 -------*/
static const struct sectree sectree_1_2 = {
	parent: &sectree_1,
	title: "医学部",
	basestr: "1Y",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_1_3 -------*/
static const struct sectree sectree_1_3 = {
	parent: &sectree_1,
	title: "社团和生活",
	basestr: "1S",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_1_4 -------*/
static const struct sectree sectree_1_4 = {
	parent: &sectree_1,
	title: "北大同学录",
	basestr: "1T",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_2 -------*/
static const struct sectree sectree_2 = {
	parent: &sectree,
	title: "电脑技术",
	basestr: "2",
	seccodes: "",
	introstr: "",
	des: "[电脑][系统]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_3 -------*/
static const struct sectree sectree_3 = {
	parent: &sectree,
	title: "学术科学",
	basestr: "3",
	seccodes: "",
	introstr: "",
	des: "[学术]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_4 -------*/
static const struct sectree sectree_4 = {
	parent: &sectree,
	title: "文化人文",
	basestr: "4",
	seccodes: "",
	introstr: "",
	des: "[艺术][文化][人文]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_5 -------*/
static const struct sectree sectree_5 = {
	parent: &sectree,
	title: "三 角 地",
	basestr: "5",
	seccodes: "",
	introstr: "",
	des: "[新闻][社会]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_6 -------*/
static const struct sectree sectree_6 = {
	parent: &sectree,
	title: "休闲娱乐",
	basestr: "6",
	seccodes: "",
	introstr: "",
	des: "[休闲][娱乐]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_7 -------*/
static const struct sectree sectree_7 = {
	parent: &sectree,
	title: "知性感性",
	basestr: "7",
	seccodes: "",
	introstr: "",
	des: "[闲聊][感性]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_8 -------*/
static const struct sectree sectree_8 = {
	parent: &sectree,
	title: "兄弟院校",
	basestr: "8",
	seccodes: "",
	introstr: "",
	des: "[院校]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9 -------*/
static const struct sectree sectree_9 = {
	parent: &sectree,
	title: "第九艺术",
	basestr: "9",
	seccodes: "DWSZTQ",
	introstr: "DWST|ZQ",
	des: "[游戏]",
	nsubsec: 6,
	subsec: {
		&sectree_9_0,
		&sectree_9_1,
		&sectree_9_2,
		&sectree_9_3,
		&sectree_9_4,
		&sectree_9_5,
	}
};
/*------ sectree_9_0 -------*/
static const struct sectree sectree_9_0 = {
	parent: &sectree_9,
	title: "Diablo专区",
	basestr: "9D",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9_1 -------*/
static const struct sectree sectree_9_1 = {
	parent: &sectree_9,
	title: "网络游戏",
	basestr: "9W",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9_2 -------*/
static const struct sectree sectree_9_2 = {
	parent: &sectree_9,
	title: "射击类游戏",
	basestr: "9S",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9_3 -------*/
static const struct sectree sectree_9_3 = {
	parent: &sectree_9,
	title: "战略游戏",
	basestr: "9Z",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9_4 -------*/
static const struct sectree sectree_9_4 = {
	parent: &sectree_9,
	title: "体育竞技游戏",
	basestr: "9T",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_9_5 -------*/
static const struct sectree sectree_9_5 = {
	parent: &sectree_9,
	title: "其他",
	basestr: "9Q",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_10 -------*/
static const struct sectree sectree_10 = {
	parent: &sectree,
	title: "糊涂特区",
	basestr: "T",
	seccodes: "",
	introstr: "",
	des: "[特区]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11 -------*/
static const struct sectree sectree_11 = {
	parent: &sectree,
	title: "原创连载",
	basestr: "Y",
	seccodes: "WHSGQL",
	introstr: "HGS|WQL",
	des: "[原创]",
	nsubsec: 6,
	subsec: {
		&sectree_11_0,
		&sectree_11_1,
		&sectree_11_2,
		&sectree_11_3,
		&sectree_11_4,
		&sectree_11_5,
	}
};
/*------ sectree_11_0 -------*/
static const struct sectree sectree_11_0 = {
	parent: &sectree_11,
	title: "武侠·军事·历史小说",
	basestr: "YW",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11_1 -------*/
static const struct sectree sectree_11_1 = {
	parent: &sectree_11,
	title: "玄幻·科幻·恐怖小说",
	basestr: "YH",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11_2 -------*/
static const struct sectree sectree_11_2 = {
	parent: &sectree_11,
	title: "社会·经济·时政系列",
	basestr: "YS",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11_3 -------*/
static const struct sectree sectree_11_3 = {
	parent: &sectree_11,
	title: "诗歌·音乐·散文集锦",
	basestr: "YG",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11_4 -------*/
static const struct sectree sectree_11_4 = {
	parent: &sectree_11,
	title: "生活·爱情·纪实故事",
	basestr: "YQ",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_11_5 -------*/
static const struct sectree sectree_11_5 = {
	parent: &sectree_11,
	title: "原创作品映射区",
	basestr: "YL",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_12 -------*/
static const struct sectree sectree_12 = {
	parent: &sectree,
	title: "信息商情",
	basestr: "X",
	seccodes: "",
	introstr: "",
	des: "[信息][商情]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_13 -------*/
static const struct sectree sectree_13 = {
	parent: &sectree,
	title: "乡情联谊",
	basestr: "H",
	seccodes: "",
	introstr: "",
	des: "[社团][群体]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_14 -------*/
static const struct sectree sectree_14 = {
	parent: &sectree,
	title: "同 学 录",
	basestr: "L",
	seccodes: "",
	introstr: "",
	des: "[同学录]",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_15 -------*/
static const struct sectree sectree_15 = {
	parent: &sectree,
	title: "体育运动",
	basestr: "S",
	seccodes: "FZ",
	introstr: "FZ",
	des: "[体育][运动]",
	nsubsec: 2,
	subsec: {
		&sectree_15_0,
		&sectree_15_1,
	}
};
/*------ sectree_15_0 -------*/
static const struct sectree sectree_15_0 = {
	parent: &sectree_15,
	title: "足球",
	basestr: "SF",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_15_1 -------*/
static const struct sectree sectree_15_1 = {
	parent: &sectree_15,
	title: "综合体育",
	basestr: "SZ",
	seccodes: "",
	introstr: "",
	des: "",
	nsubsec: 0,
	subsec: {
	}
};
/*------ sectree_16 -------*/
static const struct sectree sectree_16 = {
	parent: &sectree,
	title: "俱 乐 部",
	basestr: "C",
	seccodes: "",
	introstr: "",
	des: "[俱乐部]",
	nsubsec: 0,
	subsec: {
	}
};
