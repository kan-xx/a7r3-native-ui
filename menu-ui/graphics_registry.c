#include "graphics.h"
static int rd(const struct a7_gfx_api*a,struct a7_gfx_snapshot*s,uint32_t at,void*p,uint32_t n){
 if(!at||(at&3)||!n||n>112||at>0xffffffffu-n||s->reads>=2048||s->bytes>131072-n){s->status=2;return 0;}
 s->reads++;s->bytes+=n;if(!a->reader(a->reader_context,at,p,n)){s->status=3;return 0;}return 1;
}
static int list(const struct a7_gfx_api*a,struct a7_gfx_snapshot*s,uint32_t head,int kind){
 uint32_t h[14],end[4],n[4],item[10],first[14],entries[A7_GFX_MAX],count=0,prev=head;
 unsigned z=kind?56:36;if(!rd(a,s,head,h,z))return 0;for(unsigned i=0;i<z/4;i++)first[i]=h[i];
 if(h[0]||h[1]!=head||h[2]||h[4]||h[5]!=head||h[7]||h[8]>A7_GFX_MAX){s->status=4;return 0;}
 if(kind&&(!h[9]||!(h[13]&65535)||(h[13]&65535)>512||(h[13]>>16)!=h[8])){s->status=5;return 0;}
 uint32_t at=h[3];
 while(at!=head+16){
  if(count>=h[8]||count>=A7_GFX_MAX||!rd(a,s,at,n,16)||!n[0]||n[1]!=head||n[2]!=prev){s->status=6;return 0;}
  struct a7_gfx_entry*e=kind?&s->screen[count]:&s->layer[count];
  if(kind){
   if(at<4||n[0]!=at-4||!rd(a,s,n[0],item,40)||item[1]!=n[0]||!item[0]||!item[9]){s->status=7;return 0;}
   entries[count]=n[0];e->object=item[0];e->id=item[9];
   uint32_t obj[3];if(!rd(a,s,e->object,obj,12)||obj[2]!=e->id){s->status=8;return 0;}
  }else{
   if(n[0]>0xffffffffu-8||n[0]+8!=at||!rd(a,s,n[0],item,8)||!item[0]||!item[1]){s->status=9;return 0;}
   e->object=n[0];e->id=item[1];
  }
  for(unsigned j=0;j<count;j++){const struct a7_gfx_entry*p=kind?&s->screen[j]:&s->layer[j];if(p->id==e->id||p->object==e->object){s->status=10;return 0;}}
  count++;prev=at;at=n[3];
 }
 if(count!=h[8]||!rd(a,s,head+16,end,16)||end[0]||end[1]!=head||end[2]!=prev||end[3]||!rd(a,s,head,h,z)){s->status=11;return 0;}
 for(unsigned i=0;i<z/4;i++)if(first[i]!=h[i]){s->status=12;return 0;}
 if(kind){
  /* Native map.find has three mutable cache entries and a second intrusive
     hash linkage. Validate both indexes read-only before allowing createScreen. */
  for(unsigned i=10;i<13;i++)if(h[i]){unsigned j;for(j=0;j<count;j++)if(entries[j]==h[i])break;if(j==count){s->status=15;return 0;}}
  uint32_t seen[A7_GFX_MAX]={0},total=0,buckets=h[13]&65535;
  if(h[9]>0xffffffffu-buckets*36){s->status=16;return 0;}
  for(unsigned b=0;b<buckets;b++){
   uint32_t bh=h[9]+b*36,before[9],after[9];if(!rd(a,s,bh,before,36))return 0;
   if(before[0]||before[1]!=bh||before[2]||before[4]||before[5]!=bh||before[7]||before[8]>count){s->status=17;return 0;}
   uint32_t at=before[3],prev=bh,num=0;
   while(at!=bh+16){
    if(num>=before[8]||!rd(a,s,at,n,16)||n[1]!=bh||n[2]!=prev||!n[0]||n[0]>0xffffffffu-20||n[0]+20!=at){s->status=18;return 0;}
    unsigned j;for(j=0;j<count;j++)if(entries[j]==n[0])break;
    if(j==count||seen[j]++){s->status=19;return 0;}num++;total++;prev=at;at=n[3];
   }
   if(num!=before[8]||!rd(a,s,bh,after,36)){s->status=20;return 0;}
   for(unsigned j=0;j<9;j++)if(before[j]!=after[j]){s->status=21;return 0;}
   if(after[6]!=prev){s->status=22;return 0;}
  }
  if(total!=count){s->status=23;return 0;}s->screens=count;
 }else s->layers=count;return 1;
}
int a7_gfx_snapshot(const struct a7_gfx_api*a,struct a7_gfx_snapshot*s){
 for(unsigned i=0;i<sizeof(*s);i++)((uint8_t*)s)[i]=0;
 if(!a||!a->reader){s->status=1;return 0;}
 if(!rd(a,s,a->widget_global,&s->widget,4)||!rd(a,s,a->engine_global,&s->engine,4)||!rd(a,s,a->resource_global,&s->resource,4)||!s->widget||!s->engine||!s->resource){s->status=13;return 0;}
 uint32_t backend=0;if(s->resource>0xffffffffu-20||!rd(a,s,s->resource+16,&backend,4)||!backend){s->status=14;return 0;}
 uint32_t first_backend_word;if(!rd(a,s,backend,&first_backend_word,4)){s->status=14;return 0;}
 if(s->widget>0xffffffffu-0x4a8||s->engine>0xffffffffu-0x74||!list(a,s,s->widget+0x484,0)||!list(a,s,s->engine+0x3c,1))return 0;
 s->status=0;return 1;
}
