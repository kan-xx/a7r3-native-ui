#include "resident_route.h"
static int eq(const uint32_t*a,const uint32_t*b,uint32_t n){for(uint32_t i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static int fail(struct mpr_context*c,uint32_t code){c->report.status=code;return 0;}
static int own(struct mpr_context*c){return c->port.tid(c->port.context)==c->port.owner_tid;}
static int rd(struct mpr_context*c,uint32_t a,void*p,uint32_t n){if(!a||!n||n>112||a>0xffffffffu-n||c->report.reads>=128||c->report.bytes>8192-n)return 0;c->report.last_address=a;c->report.last_n=n;c->report.reads++;c->report.bytes+=n;return c->port.read(c->port.context,a,p,n);}
static void reset(struct mpr_context*c){c->report.reads=c->report.bytes=0;c->report.checks++;}
static int prop(struct mpr_context*c,uint32_t target){uint32_t w[12],b=c->port.view2;const uint32_t e[12]={0x8522,b+0xa3cdbc,0,0,0,0x500,b+0xa2d32c,1,0,0,target,19};return rd(c,c->port.property,w,48)&&eq(w,e,12);}
int mpr_prepare(struct mpr_context*c,const struct mpr_port*p){
 if(!c||!p||!p->read||!p->cas||!p->tid||!p->owner_tid||!p->view2||!p->caution||p->view2>0xff5932cbu||p->caution>0xff5b9ebfu||!p->route_literal||p->property!=p->view2+0xa381f4||p->flag!=p->view2+0xa6cd34)return 0;
 for(unsigned i=0;i<sizeof(*c);i++)((uint8_t*)c)[i]=0;c->port=*p;c->report.version=1;if(!own(c))return fail(c,1);reset(c);
 char literal[6];if(!prop(c,p->view2+0x62de38)||!rd(c,p->route_literal,literal,6))return fail(c,2);const char e[6]="@VFFC";for(unsigned i=0;i<6;i++)if(literal[i]!=e[i])return fail(c,2);
 c->report.prepared=1;return 1;
}
int mpr_install(struct mpr_context*c){
 if(!c||!c->report.prepared||!own(c)||c->report.installed||c->report.restored)return 0;reset(c);
 if(!prop(c,c->port.view2+0x62de38))return fail(c,3);
 if(!c->port.cas(c->port.context,c->port.property+40,c->port.view2+0x62de38,c->port.route_literal)){c->report.conflict=1;return fail(c,4);}c->report.installed=1;
 if(!prop(c,c->port.route_literal))return fail(c,5);return 1;
}
static int node(struct mpr_context*c,uint32_t a,uint32_t w[10]){uint32_t v,cb=c->port.caution;if((a&3)||!rd(c,a,w,40)||!w[0]||w[0]>0xffffff07u||(w[6]!=0xffffffffu&&w[6]>w[2])||w[9]>6)return 0;
 const uint32_t offsets[4]={0x28,0xb8,0xf0,0xf4},method[4]={0x67329d,0x673725,0x673997,0x6739c5};for(unsigned i=0;i<4;i++)if(!rd(c,w[0]+offsets[i],&v,4)||v!=cb+method[i])return 0;return 1;}
int mpr_factory_context(struct mpr_context*c,uint32_t aa){
 if(!c||!own(c)||!c->report.installed||c->report.restored)return 0;reset(c);c->report.aa=aa;c->report.leaf=0;
 uint32_t v,root,pm,w[10],next,n[10];uint8_t flag;
 if(!aa||aa>0xfffffe77u||!prop(c,c->port.route_literal)||!rd(c,c->port.flag,&flag,1)||flag!=1||!rd(c,aa,&v,4)||v!=c->port.view2+0x863890||!rd(c,aa+0x130,&v,4)||v!=1||!rd(c,aa+0x184,&root,4)||root!=c->port.view2+0xa51c60||!rd(c,aa+0x150,&pm,4)||!rd(c,pm,&v,4)||v!=c->port.view2+0x867ae0||!node(c,root,w))return fail(c,6);
 c->report.root=root;uint32_t parent=root;
 for(unsigned i=0;i<3;i++){
  if(!w[2]||w[2]>64||!w[6]||w[6]>w[2]||!w[1]||w[1]>0xffffffffu-4*w[6]||!rd(c,w[1]+4*(w[6]-1),&next,4)||!node(c,next,n)||n[4]!=parent||n[8]!=w[6]||n[9]!=2)return fail(c,7);
  for(unsigned j=0;j<10;j++)w[j]=n[j];parent=next;
 }
 uint32_t h[2],rows[18];const uint32_t expected[18]={0,0,33874048,6,0,33875023,11,1,2,12,1,4,14,1,168,20,1,0};
 if(w[0]!=c->port.caution+0x9350b0||w[3]!=c->port.caution+0xa46140||!rd(c,w[3],h,8)||h[0]!=c->port.caution+0x813a18||h[1]!=6||!rd(c,h[0],rows,72)||!eq(rows,expected,18))return fail(c,8);
 c->report.leaf=parent;c->report.status=0;return 1;
}
int mpr_restore(struct mpr_context*c){
 if(!c||!own(c)||!c->report.prepared)return 0;
 if(!c->report.installed){reset(c);if(!prop(c,c->port.view2+0x62de38))return fail(c,11);c->report.restored=1;return 1;}
 if(c->report.restored)return 1;reset(c);
 /* No overwritten third-party route. On conflict caller must hold its core. */
 if(!prop(c,c->port.route_literal)){c->report.conflict=1;return fail(c,9);}
 if(!c->port.cas(c->port.context,c->port.property+40,c->port.route_literal,c->port.view2+0x62de38)){c->report.conflict=1;return fail(c,10);}
 c->report.restored=1;if(!prop(c,c->port.view2+0x62de38))return fail(c,11);return 1;
}
int mpr_restored_current(struct mpr_context*c){if(!c||!own(c)||!c->report.prepared||!c->report.restored||c->report.conflict)return 0;reset(c);return prop(c,c->port.view2+0x62de38);}
int mpr_reinstall(struct mpr_context*c){
 if(!c)return 0;
 if(!mpr_restored_current(c))return fail(c,12);
 if(!c->port.cas(c->port.context,c->port.property+40,c->port.view2+0x62de38,c->port.route_literal)){c->report.conflict=1;return fail(c,13);}
 c->report.installed=1;c->report.restored=0;c->report.status=0;return prop(c,c->port.route_literal)?1:fail(c,14);
}
