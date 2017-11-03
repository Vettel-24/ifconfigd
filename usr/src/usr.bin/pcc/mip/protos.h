/*	$OpenBSD: protos.h,v 1.8 2008/08/17 18:40:13 ragge Exp $	*/

void cerror(char *s, ...);
void werror(char *s, ...);
void uerror(char *s, ...);
void reclaim(NODE *p, int, int);
void walkf(NODE *, void (*f)(NODE *));
void tfree(NODE *);
int tshape(NODE *, int);
void tcheck(void);
void mkdope(void);
int shtemp(NODE *p);
int flshape(NODE *p);
int shumul(NODE *p);
int ttype(TWORD t, int tword);
void expand(NODE *, int, char *);
void hopcode(int, int);
void adrcon(CONSZ);
void zzzcode(NODE *, int);
void insput(NODE *);
void upput(NODE *, int);
int andable(NODE *);
int conval(NODE *, int, NODE *);
int ispow2(CONSZ);
void defid(NODE *q, int class);
int getlab(void);
void ftnend(void);
void efcode(void);
void dclargs(void);
void cendarg(void);
int fldal(unsigned int);
int fldexpand(NODE *, int, char **);
void ecomp(NODE *p);
void bccode(void);
int upoff(int size, int alignment, int *poff);
void nidcl(NODE *p, int class);
int noinit(void);
void eprint(NODE *, int, int *, int *);
int uclass(int class);
void setregs(void);
int tlen(NODE *p);
int setbin(NODE *);
int notoff(TWORD, int, CONSZ, char *);
int notlval(NODE *);
void ecode(NODE *p);
int yylex(void);
void yyerror(char *s);
void p2tree(NODE *p);