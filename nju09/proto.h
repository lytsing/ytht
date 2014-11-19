/* BBSLIB.c */
int file_has_word(char *file, char *word);
int junkboard(char *board);
char *nohtml_textarea(const char *s);
char *nohtml(const char *s);
char *getsenv(char *s);
int http_quit(void);
void http_fatal(char *fmt, ...);
void hsprintf(char *s, char *s0);
char *titlestr(const char *s);
int hprintf(char *fmt, ...);
int fhhprintf(FCGI_FILE *output, char *fmt, ...);
void parm_add(char *name, char *val);
void parm_addfile(char *name, char *filename, char *content, int contentlen);
int isaword(char *dic[], char *buf);
int url_parse(void);
void http_parm_init(void);
int cache_header(time_t t, int age);
void html_header(int mode);
int __to16(char c);
void __unhcode(char *s);
char *getparm(char *var);
char *getparm2(char *v1, char *v2);
struct parm_file *getparmfile(char *var);
char *getparmboard(char *buf, int size);
int shm_init(struct bbsinfo *bbsinfo);
int addextraparam(char *ub, int size, int n, int param);
void extraparam_init(unsigned char *extrastr);
int user_init(struct userec **x, struct user_info **y, unsigned char *ub);
int post_mail(char *userid, char *title, char *file, char *id, char *nickname, char *ip, int sig, int mark);
int post_article_1984(char *board, char *title, char *file, char *id, char *nickname, char *ip, int sig, int mark, int outgoing, char *realauthor, int thread);
int post_article(char *board, char *title, char *file, char *id, char *nickname, char *ip, int sig, int mark, int outgoing, char *realauthor, int thread);
int securityreport(char *title, char *content);
char *anno_path_of(char *board);
int has_BM_perm(struct userec *user, struct boardmem *x);
int has_read_perm(struct userec *user, char *board);
int has_read_perm_x(struct userec *user, struct boardmem *x);
int has_view_perm(struct userec *user, char *board);
int has_view_perm_x(struct userec *user, struct boardmem *x);
int hideboard(char *bname);
int hideboard_x(struct boardmem *x);
int innd_board(char *bname);
int njuinn_board(char *bname);
int political_board(char *bname);
int anony_board(char *bname);
int noadm4political(char *bname);
int has_post_perm(struct userec *user, struct boardmem *x);
int has_vote_perm(struct userec *user, struct boardmem *x);
struct boardmem *numboard(const char *numstr);
struct boardmem *getbcache(const char *board);
int getbnumx(struct boardmem *x1);
int getbnum(char *board);
struct boardmem *getboard(char board[80]);
struct boardmem *getboard2(char board[80]);
int send_msg(char *myuserid, int i, char *touserid, int topid, char *msg, int offline);
int user_perm(struct userec *x, int level);
int count_online(void);
int loadfriend(char *id);
int initfriends(struct user_info *u);
int isfriend(char *id);
int loadbad(char *id);
int isbad(char *id);
void changemode(int mode);
char *flag_str_bm(int access);
char *flag_str(int access);
char *flag_str2(int access, int has_read);
char *userid_str(char *s);
int set_my_cookie(void);
int cmpboard(struct boardmem **b1, struct boardmem **b2);
int cmpboardscore(struct boardmem **b1, struct boardmem **b2);
int cmpboardinboard(struct boardmem **b1, struct boardmem **b2);
int cmpuser(struct user_info **a, struct user_info **b);
char *utf8_decode(char *src);
void fdisplay_attach(FCGI_FILE *output, FCGI_FILE *fp, char *currline, char *nowfile);
void printhr(void);
void updatelastboard(void);
void updateinboard(struct boardmem *x);
double *system_load(void);
int setbmstatus(struct userec *u, int online);
int count_uindex(int uid);
int dofilter(char *title, char *fn, int level);
int dofilter_edit(char *title, char *buf, int level);
int search_filter(char *pat1, char *pat2, char *pat3);
void ansi2ubb(const char *s, int size, FCGI_FILE *fp);
int ubb2ansi(const char *s, const char *fn);
void printubb(const char *form, const char *textarea);
int mmap_getline(char *ptr, int max_size);
int countln(char *fname);
char *makeurlbase(int uent, int uid);
void updatewwwindex(struct boardmem *x);
void changeContentbg(void);
/* bbsupdatelastpost.c */
int updatelastpost(char *board);
/* boardrc.c */
void brc_expired(void);
void brc_update(char *userid);
struct brcinfo *brc_readinfo(char *userid);
void brc_saveinfo(char *userid, struct brcinfo *info);
int brc_initial(char *userid, char *boardname);
int brc_uentfinial(struct user_info *uinfo);
void brc_add_read(struct fileheader *fh);
void brc_add_readt(int t);
int brc_un_read(struct fileheader *fh);
void brc_clear(void);
int brc_un_read_time(int ftime);
int brc_board_read(char *board, int ftime);
/* deny_users.c */
void loaddenyuser(char *board);
void savedenyuser(char *board);
/* bbsred.c */
char *bbsred(char *command);
/* bbsmain.c */
void logtimeused(void);
void wantquit(int signal);
struct cgi_applet *get_cgi_applet(char *needcgi);
void get_att_server(void);
int main(int argc, char *argv[]);
/* bbstop10.c */
int bbstop10_main(void);
/* bbsdoc.c */
void printdocform(char *cginame, int bnum);
void noreadperm(char *board, char *cginame);
void nosuchboard(char *board, char *cginame);
void printhrwhite(void);
void printboardhot(struct boardmem *x);
void printrelatedboard(char *board);
void printWhere(struct boardmem *x, char *str);
void printboardtop(struct boardmem *x, int num, char *infostr);
int getdocstart(int total, int lines);
void bbsdoc_helper(char *cgistr, int start, int total, int lines);
int bbsdoc_main(void);
char *size_str(int size);
char *short_Ctime(time_t t);
void docinfostr(struct boardmem *brd, int mode, int haspermm);
/* bbscon.c */
void resizecachename(char *buf, size_t len, char *fname, int ver, int pos);
int showresizecache(char *resizefn);
int showbinaryattach(char *filename);
char *binarylinkfile(char *f);
void fprintbinaryattachlink(FCGI_FILE *fp, int ano, char *attachname, int pos, int size);
int ttid(int i);
int fshowcon(FCGI_FILE *output, char *filename, int show_iframe);
int mem2html(FCGI_FILE *output, char *str, int size, char *filename);
int showcon(char *filename);
int testmozilla(void);
void processMath(void);
int bbscon_main(void);
int printReplyForm(struct boardmem *bx, struct fileheader *x, int num);
/* bbsbrdadd.c */
int bbsbrdadd_main(void);
/* bbsboa.c */
int bbsboa_main(void);
int showsecpage(const struct sectree *sec);
int showdefaultsecpage(const struct sectree *sec);
int showsechead(const struct sectree *sec);
int showstarline(char *str);
int showsecnav(const struct sectree *sec);
int genhotboard(struct hotboard *hb, const struct sectree *sec, int max);
int showhotboard(const struct sectree *sec, char *s);
int showfile(char *fn);
int showsecintro(const struct sectree *sec);
int showboardlistscript(const char *secstr);
int showboardlist(const char *secstr);
int showsecmanager(const struct sectree *sec);
/* bbsall.c */
int bbsall_main(void);
/* bbsanc.c */
int bbsanc_main(void);
/* bbs0an.c */
int anc_readtitle(FCGI_FILE *fp, char *title, int size);
int anc_readitem(FCGI_FILE *fp, char *path, int sizepath, char *name, int sizename);
int anc_hidetitle(char *title);
int bbs0an_main(void);
int getvisit(int n[2], const char *path);
/* bbslogout.c */
int bbslogout_main(void);
/* bbsleft.c */
void printdiv(int *n, char *str);
void printsectree(const struct sectree *sec);
int bbsleft_main(void);
/* bbslogin.c */
int extandipmask(int ipmask, char *ip1, char *ip2);
int bbslogin_main(void);
char *wwwlogin(struct userec *user, int ipmask);
/* bbsbadlogins.c */
int bbsbadlogins_main(void);
/* bbsqry.c */
int bbsqry_main(void);
void show_special(char *id2);
int bm_printboard(struct boardmem *bmem, char *who);
int show_onlinestate(char *userid);
/* bbsnot.c */
int bbsnot_main(void);
/* bbsfind.c */
int bbsfind_main(void);
int search(char *id, char *pat, char *pat2, char *pat3, int dt);
/* bbsfadd.c */
int bbsfadd_main(void);
/* bbsfdel.c */
int bbsfdel_main(void);
/* bbsfall.c */
int bbsfall_main(void);
/* bbsfriend.c */
int bbsfriend_main(void);
/* bbsfoot.c */
void showmyclass(void);
int bbsfoot_main(void);
int mails_time(char *id);
int mails(char *id, int *unread);
int countFriend(void);
/* bbsform.c */
int bbsform_main(void);
void check_if_ok(void);
void check_submit_form(void);
/* bbspwd.c */
int bbspwd_main(void);
/* bbsplan.c */
int bbsplan_main(void);
int save_plan(char *plan);
/* bbsinfo.c */
int bbsinfo_main(void);
int check_info(void);
/* bbsmypic.c */
int bbsmypic_main(void);
void printmypic(char *userid);
int printmypicbox(char *userid);
/* bbsmybrd.c */
int bbsmybrd_main(void);
int readmybrd(char *userid);
int ismybrd(char *board);
int read_submit(void);
/* bbssig.c */
int bbssig_main(void);
void save_sig(char *path);
/* bbspst.c */
int bbspst_main(void);
int printquote(char *filename, char *board, char *userid, int fullquote);
void printselsignature(void);
void printuploadattach(void);
void printusemath(int checked);
/* bbsgcon.c */
int bbsgcon_main(void);
/* bbsgdoc.c */
int bbsgdoc_main(void);
/* bbsdel.c */
int bbsdel_main(void);
/* bbsdelmail.c */
int bbsdelmail_main(void);
/* bbsmailcon.c */
int bbsmailcon_main(void);
/* bbsmail.c */
int bbsmail_main(void);
/* bbsdelmsg.c */
int bbsdelmsg_main(void);
/* bbssnd.c */
int testmath(char *ptr);
int bbssnd_main(void);
/* bbsnotepad.c */
int bbsnotepad_main(void);
/* bbsmsg.c */
int bbsmsg_main(void);
/* bbssendmsg.c */
int bbssendmsg_main(void);
int checkmsgbuf(char *msg);
/* bbsreg.c */
int bbsreg_main(void);
/* bbsemailreg.c */
int bbsemailreg_main(void);
/* bbsemailconfirm.c */
int doConfirm(struct userec *urec, char *email, int invited);
int bbsemailconfirm_main(void);
/* bbsinvite.c */
int bbsinvite_main(void);
/* bbsinviteconfirm.c */
int bbsinviteconfirm_main(void);
/* bbsmailmsg.c */
int bbsmailmsg_main(void);
void mail_msg(struct userec *user);
/* bbssndmail.c */
int bbssndmail_main(void);
/* bbsnewmail.c */
int bbsnewmail_main(void);
/* bbspstmail.c */
int bbspstmail_main(void);
/* bbsgetmsg.c */
int print_emote_table(char *form, char *input);
int emotion_print(char *msg);
int bbsgetmsg_main(void);
void check_msg(void);
/* bbscloak.c */
int bbscloak_main(void);
/* bbsmdoc.c */
int bbsmdoc_main(void);
/* bbsnick.c */
int bbsnick_main(void);
/* bbstfind.c */
int bbstfind_main(void);
/* bbsadl.c */
int bbsadl_main(void);
/* bbstcon.c */
int bbstcon_main(void);
int fshow_file(FCGI_FILE *output, char *board, struct fileheader *x);
int show_file(struct boardmem *brd, struct fileheader *x, int num, int floor);
/* bbstdoc.c */
int bbstdoc_main(void);
/* bbsdoreg.c */
int verifyInvite(char *email);
int postInvite(char *userid, char *inviter);
int checkRegPass(char *iregpass, const char *userid);
int bbsdoreg_main(void);
int badstr(unsigned char *s);
void newcomer(struct userec *x, char *words);
/* bbsmywww.c */
int bbsmywww_main(void);
int save_set(int t_lines, int link_mode, int def_mode, int att_mode, int doc_mode, int edit_mode);
/* bbsccc.c */
int bbsccc_main(void);
int do_ccc(struct fileheader *x, struct boardmem *brd1, struct boardmem *brd);
/* bbsclear.c */
int bbsclear_main(void);
/* bbsstat.c */
int search_stat(void *ptr, int size, int key);
int bbsstat_main(void);
/* bbsedit.c */
int bbsedit_main(void);
int Origin2(char text[256]);
int update_form(char *board, char *file, char *title);
int getpathsize(char *path);
/* bbsman.c */
int bbsman_main(void);
int do_del(char *board, char *file);
int do_set(char *dirptr, int size, char *file, int flag, char *board);
/* bbsparm.c */
int bbsparm_main(void);
int read_form(void);
/* bbsfwd.c */
int bbsfwd_main(void);
int do_fwd(struct fileheader *x, char *board, char *target);
/* bbsmnote.c */
int bbsmnote_main(void);
void save_note(char *path);
/* bbsdenyall.c */
int bbsdenyall_main(void);
/* bbsdenydel.c */
int bbsdenydel_main(void);
/* bbsdenyadd.c */
int bbsdenyadd_main(void);
/* bbstopb10.c */
int bbstopb10_main(void);
/* bbsbfind.c */
int bbsbfind_main(void);
/* bbsx.c */
int bbsx_main(void);
/* bbseva.c */
int bbseva_main(void);
int set_eva(char *board, char *file, int star, int result[2], char *buf);
int do_eva(char *board, char *file, int star);
/* bbsvote.c */
int bbsvote_main(void);
int addtofile(char filename[STRLEN], char str[256]);
int valid_voter(char *board, char *name);
/* bbsshownav.c */
int bbsshownav_main(void);
int shownavpart(int mode, const char *secstr);
void shownavpartline(char *buf, int mode);
/* bbsbkndoc.c */
int bbsbkndoc_main(void);
/* bbsbknsel.c */
int bbsbknsel_main(void);
/* bbsbkncon.c */
int bbsbkncon_main(void);
/* bbshome.c */
char *userid_str2(char *s);
int bbshome_main(void);
/* bbsindex.c */
int checkfile(char *fn, int maxsz);
int loadoneface(void);
int showannounce(void);
void loginwindow(void);
int setlastip(void);
void shownologin(void);
void checklanguage(void);
int bbsindex_main(void);
/* bbslform.c */
int bbslform_main(void);
/* regreq.c */
int regreq_main(void);
/* bbsselstyle.c */
int bbsselstyle_main(void);
/* bbscon1.c */
int bbscon1_main(void);
/* bbsattach.c */
int bbsattach_main(void);
/* bbskick.c */
int bbskick_main(void);
/* bbsrss.c */
char *nonohtml(const char *s);
int bbsrss_main(void);
/* bbsscanreg.c */
int bbsscanreg_main(void);
/* bbsshowfile.c */
int bbsshowfile_main(void);
/* bbsdt.c */
int bbsdt_main(void);
/* bbslt.c */
void print_radio(char *cname, char *name, char *str[], int len, int select);
int bbslt_main(void);
/* bbsincon.c */
int bbsincon_main(void);
/* bbssetscript.c */
int bbssetscript_main(void);
/* bbscccmail.c */
int bbscccmail_main(void);
int do_cccmail(struct fileheader *x, struct boardmem *brd);
/* bbsfwdmail.c */
int bbsfwdmail_main(void);
int do_fwdmail(char *fn, struct fileheader *x, char *target);
/* bbsscanreg_findsurname.c */
int bbsscanreg_findsurname_main(void);
/* bbsnt.c */
int bbsnt_main(void);
/* bbstopcon.c */
int bbstopcon_main(void);
/* bbsdrawscore.c */
int bbsdrawscore_main(void);
int printparam(int n, char *board);
/* bbsmyclass.c */
void showmyclasssetting(void);
void savemyclass(void);
int bbsmyclass_main(void);
/* bbssearchboard.c */
int bbssearchboard_main(void);
/* bbslastip.c */
int bbslastip_main(void);
/* bbsucss.c */
int bbsucss_main(void);
/* bbsdefcss.c */
int bbsdefcss_main(void);
/* bbsself_photo_vote.c */
int bbsself_photo_vote_main(void);
/* bbsspam.c */
int bbsspam_main(void);
/* bbsspamcon.c */
int bbsspamcon_main(void);
/* bbssouke.c */
int bbssouke_main(void);
void printSoukeForm(void);
/* bbsboardlistscript.c */
int bbsboardlistscript_main(void);
int listmybrd(struct boardmem *(data[]));
int makeboardlist(const struct sectree *sec, struct boardmem *(data[]));
int boardlistscript(struct boardmem *(data[]), int total);
int oneboardscript(struct boardmem *brd);
/* bbsolympic.c */
int bbsolympic_main(void);
int showolympic(void);
int showlastanc(char *buf);
int showfirstanc(char *buf);
int showoneboardscript(char *aboard);
int shownewpost(char *board);
int shownewmark(char *board);
/* bbsicon.c */
int bbsicon_main(void);
/* bbstmpleft.c */
int bbstmpleft_main(void);
/* bbsdlprepare.c */
int bbsdlprepare_main(void);
int trySetDLPass(char *taskfile);
int printSetDLPass(void);
int reportStatus(char *taskfile);
/* bbsnewattach.c */
int bbsnewattach_main(void);
/* bbsupload.c */
int save_attach(char *path, int totalsize);
void printuploadform(void);
int bbsupload_main(void);

int bbsthdoc_main(void);
int bbsthcon_main(void);

/* blogblog.c */
void printXBlogHeader(void);
void printXBlogEnd(void);
void printBlogHeader(struct Blog *blog);
void printBlogSideBox(struct Blog *blog);
void printBlogFooter(struct Blog *blog);
int printAbstract(struct Blog *blog, int n);
void printAbstractsTime(struct Blog *blog, int start, int end);
void printPages(char *link, int count, int currPage);
void printAbstractsSubject(struct Blog *blog, int subject, int page);
int tagedAs(struct BlogHeader *blh, int tag);
void printAbstractsTag(struct Blog *blog, int tag, int page);
void printAbstractsAll(struct Blog *blog, int page);
int blogblog_main(void);
/* blogread.c */
int printPrevNext(struct Blog *blog, int n);
int printBlogArticle(struct Blog *blog, int n);
void printCommentFile(char *fileName, int commentTime, int i);
void printComments(struct Blog *blog, int fileTime);
void printCommentBox(struct Blog *blog, int fileTime);
void print_add_new_comment(struct Blog *blog, int fileTime);
int blogread_main(void);
/* blogpost.c */
void printUploadUTF8(void);
void printSelectSubject(struct Blog *blog, struct BlogHeader *blh);
void printSelectTag(struct Blog *blog, struct BlogHeader *blh);
void printTextarea(char *text, int size);
int blogpost_main(void);
/* blogsend.c */
int saveTidyHtml(char *filename, char *content, int filterlevel);
char *readFile(char *filename);
int blogsend_main(void);
/* blogeditabstract.c */
void printBlogEditPostSideBox(struct Blog *blog, time_t fileTime);
void printAbstractBox(struct Blog *blog, int n);
int doSaveAbstract(struct Blog *blog, time_t fileTime, char *abstract);
int blogeditabstract_main(void);
/* blogeditconfig.c */
void modifyConfig(struct Blog *blog, int echo);
void printBlogSettingSideBox(struct Blog *blog);
int blogeditconfig_main(void);
/* blogeditsubject.c */
int blogeditsubject_main(void);
/* blogedittag.c */
int blogedittag_main(void);
/* blogeditpost.c */
int blogeditpost_main(void);
/* blogdraft.c */
int printDraftAbstract(struct BlogHeader *blh);
int blogdraft_main(void);
/* blogdraftread.c */
int printDraftArticle(struct Blog *blog, int n);
int blogdraftread_main(void);
/* blogcomment.c */
int blogcomment_main(void);
/* blogsetup.c */
void printLoginFormUTF8(void);
int blogsetup_main(void);
/* blogpage.c */
int blogpage_main(void);
/* blogrss2.c */
char *gmtctime(const time_t *timep);
int blogrss2_main(void);
/* blogatom.c */
char *gmtctime(const time_t *timep);
int blogatom_main(void);
/* blog.c */ 
void setBlogFile(char *buf, char *userid, char *file); 
void setBlogPost(char *buf, char *userid, time_t fileTime); 
void setBlogAbstract(char *buf, char *userid, time_t fileTime); 
void setBlogCommentIndex(char *buf, char *userid, time_t fileTime); 
void setBlogComment(char *buf, char *userid, time_t fileTime,time_t commentTime); 
int createBlog(char *userid); 
int openBlog(struct Blog *blog, char *userid); 
int openBlogW(struct Blog *blog, char *userid); 
int openBlogD(struct Blog *blog, char *userid); 
void closeBlog(struct Blog *blog); 
void reopenBlog(struct Blog *blog); 
int blogPost(struct Blog *blog, char *title, char *tmpfile,
int subject, int tags[], int nTag); 
int blogSaveAbstract(struct Blog *blog, time_t fileTime, char *abstract); 
int blogUpdatePost(struct Blog *blog, time_t fileTime, char
*title, char *tmpfile, int subject, int tags[], int nTag); 
int blogPostDraft(struct Blog *blog, char *title, char *tmpfile, int draftID); 
int deleteDraft(struct Blog *blog, int draftID); 
int blogComment(struct Blog *blog, time_t fileTime, char *tmpfile); 
int blogCheckMonth(struct Blog *blog, char buf[32], int year,
int month); 
int blogModifySubject(struct Blog *blog, int subjectID, char *title, int hide); 
int blogModifyTag(struct Blog *blog, int tagID, char *title,
int hide); 
int findBlogArticle(struct Blog *blog, time_t fileTime); 
void blogLog(char *fmt, ...); 
/* strop.c */ 
char *utf8cut(char *str, int maxsize); 
char *gb2utf8(char *to0, int tolen0, char *from0); 

