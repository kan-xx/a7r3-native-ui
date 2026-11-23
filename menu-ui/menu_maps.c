#include "menu_maps.h"
static int prefix(const char *p,const char *e,const char *s){
 while(*s){if(p==e||*p++!=*s++)return 0;}return 1;
}
static int exact(const char*p,const char*e,const char*s){
 while(*s){if(p==e||*p++!=*s++)return 0;}return p==e;
}
static int hex(const char **pp,const char *e,uint32_t *v){
 const char*p=*pp;uint32_t x=0,n=0;
 while(p<e){unsigned c=(unsigned char)*p,d;
  if(c>='0'&&c<='9')d=c-'0';else if(c>='a'&&c<='f')d=c-'a'+10;else if(c>='A'&&c<='F')d=c-'A'+10;else break;
  if(++n>8)return 0;x=x*16+d;p++;
 }
 if(!n)return 0;*v=x;*pp=p;return 1;
}
static void space(const char **p,const char*e){while(*p<e&&(**p==' '||**p=='\t'))(*p)++;}
static int number(const char **p,const char *e,int radix){const char*s=*p;
 while(*p<e){char c=**p;if((c>='0'&&c<='9')||(radix==16&&((c>='a'&&c<='f')||(c>='A'&&c<='F'))))(*p)++;else break;}
 return *p>s;
}
static int library(const char *p,const char *e){
 if(!prefix(p,e,"/usr/lib/")&&!prefix(p,e,"/lib/"))return 0;
 /* maps uses the resolved backing pathname, so /dev aliases are excluded.
  * Admit shared-library files only; no arbitrary files or other special maps. */
 for(const char*q=p;e-q>=3;q++)if(q[0]=='.'&&q[1]=='s'&&q[2]=='o'&&
    (q+3==e||q[3]=='.'||q[3]==' '))return 1;
 return 0;
}
int a7_menu_parse_maps(const char *b,uint32_t n,struct a7_menu_maps *m){
 if(!b||!m||!n||n>A7_MENU_MAP_TEXT_MAX||b[n-1]!='\n')return 0;
 m->count=0;m->lines=0;const char*p=b,*end=b+n;uint32_t previous=0;
 while(p<end){const char*e=p;while(e<end&&*e!='\n')e++;if(++m->lines>A7_MENU_MAP_LINES_MAX)return 0;
  uint32_t low,high;if(!hex(&p,e,&low)||p==e||*p++!='-'||!hex(&p,e,&high)||low>=high||low<previous)return 0;
  previous=high;if(p==e||*p!=' ')return 0;space(&p,e);
  if(e-p<5)return 0;const char*perms=p;p+=4;if(*p!=' ')return 0;
  if((perms[0]!='r'&&perms[0]!='-')||(perms[1]!='w'&&perms[1]!='-')||
     (perms[2]!='x'&&perms[2]!='-')||(perms[3]!='p'&&perms[3]!='s'))return 0;
  /* Offset, device and inode are mandatory separate columns. */
  space(&p,e);if(!number(&p,e,16)||p==e||*p!=' ')return 0;
  space(&p,e);if(!number(&p,e,16)||p==e||*p++!=':'||!number(&p,e,16)||p==e||*p!=' ')return 0;
  space(&p,e);if(!number(&p,e,10)||(p<e&&*p!=' '&&*p!='\t'))return 0;
  space(&p,e);
  int anon=p==e,heap=exact(p,e,"[heap]"),stack=exact(p,e,"[stack]")||prefix(p,e,"[stack:");
  int private_rw=perms[0]=='r'&&perms[1]=='w'&&perms[2]=='-'&&perms[3]=='p';
  int admitted=(private_rw&&(anon||heap||stack))||
    (perms[0]=='r'&&perms[3]=='p'&&!(perms[1]=='w'&&perms[2]=='x')&&library(p,e));
  if(admitted){if(m->count==A7_MENU_MAP_RANGES_MAX)return 0;m->ranges[m->count++]=(struct a7_menu_range){low,high,private_rw};}
  p=e+1;
 }
 return m->count!=0;
}
int a7_menu_maps_contains(const struct a7_menu_maps*m,uint32_t a,uint32_t n){
 if(!m||!n||m->count>A7_MENU_MAP_RANGES_MAX||a>UINT32_MAX-n)return 0;
 for(uint32_t i=0;i<m->count;i++)if(a>=m->ranges[i].begin&&a+n<=m->ranges[i].end)return 1;
 return 0;
}

int a7_menu_maps_writable(const struct a7_menu_maps*m,uint32_t a,uint32_t n){
 if(!m||!n||m->count>A7_MENU_MAP_RANGES_MAX||a>UINT32_MAX-n)return 0;
 for(uint32_t i=0;i<m->count;i++)if(m->ranges[i].writable&&a>=m->ranges[i].begin&&a+n<=m->ranges[i].end)return 1;
 return 0;
}
