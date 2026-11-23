#include "menu_snapshot.h"
struct reading {a7_menu_read read;void *context;struct a7_menu_snapshot *out;};
static int fetch(struct reading *r,uint32_t p,void *dst,uint32_t n) {
 if(r->out->reads>=2048||n>65536-r->out->bytes) {
  r->out->status=A7_MENU_READ_BUDGET;r->out->failed_address=p;return 0;
 }
 if (!p||!n||p>UINT32_MAX-n||!r->read(r->context,p,dst,n)) {
  r->out->failed_address=p;r->out->status=A7_MENU_UNREADABLE;return 0;
 }
 r->out->reads++;r->out->bytes+=n;return 1;
}
static int aligned(uint32_t p) {return p&&!(p&3);}
static int text(struct reading *r,uint32_t p,char dst[A7_MENU_TEXT]) {
 if (!p) return 1;
 /* Never cross a 4K page in a string read; stop after the first NUL. */
 for (uint32_t i=0;i<A7_MENU_TEXT-1;) {
  uint32_t n=16,page_left=4096-((p+i)&4095);
  if(n>page_left)n=page_left;if(n>A7_MENU_TEXT-1-i)n=A7_MENU_TEXT-1-i;
  if(p+i<p||!fetch(r,p+i,dst+i,n))return 0;
  for(uint32_t j=0;j<n;j++)if(!dst[i+j]){
   for(uint32_t k=i+j+1;k<i+n;k++)dst[k]=0;return 1;
  }
  i+=n;
 }
 dst[A7_MENU_TEXT-1]=0;return 1;
}
int a7_menu_take_snapshot(a7_menu_read read,void *context,uint32_t global,
 uint32_t singleton,uint32_t selected,struct a7_menu_snapshot *o) {
 if(!read||!o||!aligned(global)||!aligned(singleton))return A7_MENU_BAD_ARG;
 unsigned char *clear=(unsigned char*)o;
 for(size_t i=0;i<sizeof(*o);i++)clear[i]=0;
 o->version=1;o->selected_id=selected;
 struct reading r={read,context,o};uint32_t m[28],t[3],slot[4],node[6];
 if(!fetch(&r,global,&o->manager,4))return o->status;
 if(!o->manager){o->status=A7_MENU_NO_MANAGER;return o->status;}
 if(!aligned(o->manager)||!fetch(&r,o->manager,m,sizeof(m)))goto bad_manager;
 o->configured_slots=m[0];o->registry=m[2];o->app_config=m[7];
 o->registry_is_singleton=o->registry==singleton;
 if(!m[0]||m[0]>A7_MENU_MAX_SLOTS||!aligned(m[2])||
    !aligned(m[8])||!aligned(m[9]))goto bad_manager;
 if(!fetch(&r,m[2],t,sizeof(t)))return o->status;
 o->registry_vptr=t[0];o->sentinel=m[2]+4;o->tail=t[2];
 for(uint32_t bank=0;bank<2;bank++)for(uint32_t i=0;i<m[0];i++) {
  uint32_t at=m[8+bank]+16*i;
  if(at<m[8+bank]||!fetch(&r,at,slot,sizeof(slot)))return o->status;
  if(!slot[1])continue;
  struct a7_menu_view *v=&o->active[o->active_count++];
  v->bank=bank;v->index=i;v->handle=slot[0];v->view=slot[1];v->id=slot[2];v->library=slot[3];
  if(!aligned(v->view))goto bad_manager;
  if(!fetch(&r,v->view,&v->vptr,4))return o->status;
 }
 uint32_t p=t[1],prev=o->sentinel;
 for(uint32_t count=0;p!=o->sentinel;count++) {
  if(count==A7_MENU_MAX_NODES){o->status=A7_MENU_NODE_LIMIT;return o->status;}
  if(!aligned(p)){o->status=A7_MENU_BAD_LINK;return o->status;}
  if(!fetch(&r,p,node,sizeof(node)))return o->status;
  if(node[1]!=prev||!aligned(node[0])){o->status=A7_MENU_BAD_LINK;return o->status;}
  o->registry_count++;
  if(node[3]==selected) {
   if(o->selected_count++){o->status=A7_MENU_DUPLICATE_ID;return o->status;}
   o->selected_node=p;o->selected_module_ptr=node[4];o->selected_factory_ptr=node[5];
   if(!text(&r,node[4],o->selected_module)||!text(&r,node[5],o->selected_factory))return o->status;
  }
  for(uint32_t i=0;i<o->active_count;i++)if(o->active[i].id==node[3]) {
   if(o->active[i].registry_matches++){o->status=A7_MENU_DUPLICATE_ID;return o->status;}
   o->active[i].registered_module=node[4];o->active[i].registered_factory=node[5];
  }
  prev=p;p=node[0];
 }
 if(prev!=o->tail){o->status=A7_MENU_BAD_LINK;return o->status;}
 for(uint32_t i=0;i<o->active_count;i++)if(o->active[i].registry_matches) {
  if(!text(&r,o->active[i].registered_module,o->active[i].module)||
     !text(&r,o->active[i].registered_factory,o->active[i].factory))return o->status;
 }
 return o->status;
bad_manager:
 if(!o->status)o->status=A7_MENU_BAD_MANAGER;return o->status;
}
