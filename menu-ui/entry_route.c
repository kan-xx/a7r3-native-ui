#include "entry_route.h"
static const char route_name[]="@VFFC";
static int eq(const uint32_t*a,const uint32_t*b,uint32_t n){for(uint32_t i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static int readn(struct a7_route_context*c,uint32_t a,void*p,uint32_t n){
 if(!a||!n||n>112||a>0xffffffffu-n||c->report.reads>=8192||c->report.bytes>196608-n)return 0;
 c->report.last_address=a;c->report.last_n=n;c->report.reads++;c->report.bytes+=n;return c->api.read(c->api.context,a,p,n);
}
static int fail(struct a7_route_context*c,uint32_t s){c->report.status=s;return 0;}
static int node(struct a7_route_context*c,uint32_t a,struct a7_route_node*n){
 uint32_t methods[4],cb=c->api.caution;
 if((a&3)||!readn(c,a,n->word,40))return 0;n->address=a;
 /* Original initSettingNode writes signed -1 to +0x18 for unselected
  * descendants (673a94..98); getSelectedItem treats <=0 as none. Keep
  * every other field bound, and validate actual selected parents below. */
 if(!n->word[0]||n->word[0]>0xffffffffu-0xf8||(n->word[6]!=0xffffffffu&&n->word[6]>n->word[2])||n->word[9]>6)return 0;
 if(!readn(c,n->word[0]+0x28,methods,4)||methods[0]!=cb+0x67329d||
 !readn(c,n->word[0]+0xb8,methods+1,4)||methods[1]!=cb+0x673725||
 !readn(c,n->word[0]+0xf0,methods+2,4)||methods[2]!=cb+0x673997||
 !readn(c,n->word[0]+0xf4,methods+3,4)||methods[3]!=cb+0x6739c5)return 0;
 return 1;
}
static int child(struct a7_route_context*c,const struct a7_route_node*p,uint32_t i,struct a7_route_node*out){
 uint32_t a=0;if(p->word[2]>64||!i||i>p->word[2]||!p->word[1]||p->word[1]>0xffffffffu-4*i||!readn(c,p->word[1]+4*(i-1),&a,4)||!node(c,a,out))return 0;
 return out->word[4]==p->address&&out->word[8]==i;
}
static int procid(struct a7_route_context*c,const struct a7_route_node*n,uint32_t*out){
 uint32_t p[2],rows[27],found=0;*out=0xffffffffu;
 if(!n->word[3])return 1;
 if(!readn(c,n->word[3],p,8)||p[1]>128)return 0;
 /* Original constructor stores the number of 12-byte records. The original
  * binary getter uses an inclusive upper bound; some original lists are also
  * not sorted. This is bounded read-only identification, never its replacement:
  * inspect only count records, in <=108-byte chunks, and reject duplicate14. */
 if(!p[1])return 1;
 if(!p[0]||p[0]>0xffffffffu-12*p[1])return 0;
 for(uint32_t first=0;first<p[1];){
  uint32_t count=p[1]-first;if(count>9)count=9;
  if(!readn(c,p[0]+12*first,rows,12*count))return 0;
  for(uint32_t i=0;i<count;i++)if(rows[3*i]==14){if(found++)return 0;*out=rows[3*i+2];}
  first+=count;
 }
 return 1;
}
static int save(struct a7_route_context*c,const struct a7_route_node*n){
 for(unsigned i=0;i<c->saved_count;i++)if(c->saved[i].address==n->address)return eq(c->saved[i].word,n->word,10);
 if(c->saved_count>=10)return 0;c->saved[c->saved_count++]=*n;return 1;
}
static int path_walk(struct a7_route_context*c,const struct a7_route_node*p,unsigned depth,uint32_t*path){
 /* Only root/tab/page child arrays are expanded; a level-2 item may
  * contain a much longer value list (native example91), never visited here. */
 if(p->word[2]>64)return 0;
 for(uint32_t i=1;i<=p->word[2];i++){
  struct a7_route_node n;if(++c->report.nodes>1024||!child(c,p,i,&n))return 0;path[depth]=n.address;
  if(depth<2){if(!path_walk(c,&n,depth+1,path))return 0;}
  else{uint32_t id;if(!procid(c,&n,&id))return 0;if(id==0xa8){
    if(n.word[0]!=c->api.caution+0x9350b0)return 0;c->report.matches++;for(unsigned j=0;j<3;j++)c->path[j]=path[j];
  }}
 }return 1;
}
static int aa_live(struct a7_route_context*c){uint32_t x[2],mode=0,root=0,pm=0,vp=0;
 return readn(c,c->aa,x,4)&&x[0]==c->aa_vptr&&readn(c,c->aa+0x130,&mode,4)&&mode==1&&
 readn(c,c->aa+0x184,&root,4)&&root==c->root&&readn(c,c->aa+0x150,&pm,4)&&pm==c->manager&&
 readn(c,pm,&vp,4)&&vp==c->api.view2+0x867ae0;
}
int a7_route_prepare(struct a7_route_context*c,const struct a7_route_api*a,uint32_t aa,uint32_t vp){
 c->api=*a;c->report.version=5;c->aa=aa;c->aa_vptr=vp;c->report.step=1;
 uint32_t mode=0;uint8_t flag=0;
 if(!a->read||!a->cas||!a->select||!a->action||!a->set_flag||vp!=a->view2+0x863890||
 !readn(c,aa+0x130,&mode,4)||mode!=1||!readn(c,aa+0x184,&c->root,4)||!readn(c,aa+0x150,&c->manager,4)||!aa_live(c)||
 !readn(c,a->flag,&flag,1)||flag>1)return fail(c,1);
 c->report.flag_before=flag;c->report.root=c->root;c->report.step=2;
 if(!readn(c,a->property,c->property,48)||c->property[0]!=0x8522||c->property[1]!=a->view2+0xa3cdbc||c->property[2]||c->property[3]||c->property[4]||c->property[5]!=0x500||c->property[6]!=a->view2+0xa2d32c||c->property[7]!=1||c->property[8]||c->property[9]||c->property[10]!=a->original_target||c->property[11]!=19)return fail(c,2);
 struct a7_route_node root;uint32_t path[3];if(!node(c,c->root,&root)||!path_walk(c,&root,0,path)||c->report.matches!=1)return fail(c,3);
 c->report.step=3;struct a7_route_node parent=root;
 for(unsigned i=0;i<3;i++){
  struct a7_route_node n,old;/* Only the three parents we will mutate need an existing selected child. */
  if(parent.word[2]>64||!parent.word[6]||parent.word[6]>parent.word[2])return fail(c,4);
  if(!node(c,c->path[i],&n)||!child(c,&parent,parent.word[6],&old)||n.word[4]!=parent.address||
    (n.word[9]!=1&&n.word[9]!=2)||(old.word[9]!=2&&old.word[9]!=4&&old.word[9]!=6)||!save(c,&parent)||!save(c,&old)||!save(c,&n))return fail(c,4);
  c->parents[i]=parent.address;c->old[i]=old.address;c->report.path[i]=n.address;c->report.old[i]=old.address;parent=n;
 }
 return 1;
}
static int restore_pointer(struct a7_route_context*c){
 if(!c->report.route_installed||c->report.route_restored)return 1;
 if(!c->api.cas(c->api.context,c->api.property+40,(uint32_t)(uintptr_t)route_name,c->api.original_target))return fail(c,12);
 c->report.route_restored=1;return 1;
}
int a7_route_execute(struct a7_route_context*c){
 c->report.step=4;if(!aa_live(c))return fail(c,5);
 for(unsigned i=0;i<c->saved_count;i++){uint32_t words[10];if(!readn(c,c->saved[i].address,words,40)||!eq(words,c->saved[i].word,10))return fail(c,5);}
 for(unsigned i=0;i<3;i++){
  c->dirty=1;if(!a7_route_select_caught(c->api.select,(void*)(uintptr_t)c->path[i],&c->report.exception))return fail(c,6);c->report.selected++;
 }
 struct a7_route_node parent;if(!node(c,c->root,&parent))return fail(c,7);
 for(unsigned i=0;i<3;i++){struct a7_route_node n;if(!child(c,&parent,parent.word[6],&n)||n.address!=c->path[i]||n.word[9]!=2)return fail(c,7);parent=n;}
 uint32_t id;if(!procid(c,&parent,&id)||id!=0xa8)return fail(c,7);
 uint32_t property[12];if(!readn(c,c->api.property,property,48)||!eq(property,c->property,12))return fail(c,8);
 if(!c->api.persistent_route){if(!c->api.cas(c->api.context,c->api.property+40,c->api.original_target,(uint32_t)(uintptr_t)route_name))return fail(c,8);c->report.route_installed=1;}c->report.step=5;c->report.action_called=1;
 c->report.action_returned=a7_route_action_caught(c->api.action,(void*)(uintptr_t)c->aa,&c->report.exception);
 /* IdGenerator and ParamInt copy target ID/mode before the original call returns.
  * Restore route in this same callback even if original action throws. */
 int restored=restore_pointer(c);uint8_t flag=0;if(readn(c,c->api.flag,&flag,1))c->report.flag_after=flag;else return fail(c,9);
 if(!restored||!c->report.action_returned||c->report.exception||flag!=1)return fail(c,10);
 return 1;
}
int a7_route_restore(struct a7_route_context*c){
 if(!restore_pointer(c))return 0;if(!c->dirty||c->report.restored)return 1;
 c->report.step=6;if(!aa_live(c)||c->report.exception)return fail(c,11);
 /* Never overwrite a concurrently changed property/parent/state. Qualified
  * original setter is the only writer of selection; its old selected item is
  * re-read and its identity remains the expected path entry. */
 uint32_t eligibility[10];
 for(unsigned i=0;i<c->saved_count;i++){uint32_t w[10];if(!readn(c,c->saved[i].address,w,40))return fail(c,11);for(unsigned j=0;j<10;j++)if(j!=6&&j!=9&&w[j]!=c->saved[i].word[j])return fail(c,11);if(w[9]<1||w[9]>6)return fail(c,11);eligibility[i]=(w[9]-1)/2;}
 for(unsigned i=0;i<c->report.selected;i++){
  struct a7_route_node p,n;if(!node(c,c->parents[i],&p)||!child(c,&p,p.word[6],&n)||n.address!=c->path[i]||(n.word[9]!=2&&n.word[9]!=4&&n.word[9]!=6))return fail(c,11);
 }
 for(int i=(int)c->report.selected-1;i>=0;i--)if(!a7_route_select_caught(c->api.select,(void*)(uintptr_t)c->old[i],&c->report.exception))return fail(c,11);
 /* Original isItemSelected accepts2/4/6, while setItemGrayout preserves
  * selection and changes eligibility1/2 <->3/4 <->5/6. Keep current eligibility;
  * restore only selected identity and exact original structural/index fields. */
 for(unsigned i=0;i<c->saved_count;i++){uint32_t words[10];if(!readn(c,c->saved[i].address,words,40))return fail(c,11);for(unsigned j=0;j<9;j++)if(words[j]!=c->saved[i].word[j])return fail(c,11);if(words[9]<1||words[9]>6||(words[9]-1)/2!=eligibility[i]||((words[9]&1)!=(c->saved[i].word[9]&1)))return fail(c,11);}
 c->report.restored=1;c->dirty=0;return 1;
}
int a7_route_finish_flag(struct a7_route_context*c){
 if(!c->report.action_called)return 1;if(c->report.route_installed&&!c->report.route_restored)return fail(c,13);if(!c->report.restored||c->report.exception)return fail(c,13);
 /* Observe the original close result. CommonShootingSetting is a source
  * context flag, not an AA existence count. Do not write a historical baseline. */
 uint8_t flag=0;if(!readn(c,c->api.flag,&flag,1)||flag>1)return fail(c,13);c->report.flag_after=flag;c->report.flag_restored=flag==c->report.flag_before;c->report.flag_restore_write=0;c->report.step=7;return 1;
}
