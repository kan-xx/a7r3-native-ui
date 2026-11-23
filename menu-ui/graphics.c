#include "graphics.h"
static uint32_t tid(void){register uint32_t r0 __asm__("r0");register uint32_t r7 __asm__("r7")=224;__asm__ volatile("svc 0":"=r"(r0):"r"(r7):"memory","cc");return r0;}
static int same_entries(const struct a7_gfx_snapshot*a,const struct a7_gfx_snapshot*b){
 if(a->layers!=b->layers||a->screens!=b->screens||a->widget!=b->widget||a->engine!=b->engine||a->resource!=b->resource)return 0;
 for(unsigned i=0;i<a->layers;i++)if(a->layer[i].id!=b->layer[i].id||a->layer[i].object!=b->layer[i].object)return 0;
 for(unsigned i=0;i<a->screens;i++)if(a->screen[i].id!=b->screen[i].id||a->screen[i].object!=b->screen[i].object)return 0;
 return 1;
}
static int absent(const struct a7_gfx_snapshot*s){for(unsigned i=0;i<s->layers;i++)if(s->layer[i].id==A7_LAYER_ID)return 0;for(unsigned i=0;i<s->screens;i++)if(s->screen[i].id==A7_LAYER_ID)return 0;return 1;}
static int ids(const struct a7_gfx_api*a){uint32_t d,h,i;
 return a->reader(a->reader_context,a->default_id,&d,4)&&a->reader(a->reader_context,a->hidden_id,&h,4)&&a->reader(a->reader_context,a->invisible_id,&i,4)&&d&&h&&i&&A7_LAYER_ID!=d&&A7_LAYER_ID!=h&&A7_LAYER_ID!=i;
}
static int reserved_layers(const struct a7_gfx_api*a,const struct a7_gfx_snapshot*s){uint32_t d,h;unsigned dc=0,hc=0;
 if(!a->reader(a->reader_context,a->default_id,&d,4)||!a->reader(a->reader_context,a->hidden_id,&h,4))return 0;
 for(unsigned j=0;j<s->layers;j++){dc+=s->layer[j].id==d;hc+=s->layer[j].id==h;}return dc==1&&hc==1;
}
int a7_gfx_prepare(struct a7_gfx_context*c,const struct a7_gfx_api*a,uint32_t owner){
 if(!c||!a||!owner)return 0;for(unsigned i=0;i<sizeof(*c);i++)((uint8_t*)c)[i]=0;c->api=*a;c->report.version=1;c->report.owner_tid=owner;c->report.font_id=0xffffffffu;
 if(tid()!=owner){c->report.thread_errors++;c->report.status=20;return 0;}
 if(!ids(a)||!a7_gfx_snapshot(a,&c->before)||!absent(&c->before)||!reserved_layers(a,&c->before)){c->report.status=c->before.status?100+c->before.status:21;c->report.reads=c->before.reads;c->report.bytes=c->before.bytes;return 0;}
 c->report.pre_valid=1;c->report.before_layers=c->before.layers;c->report.before_screens=c->before.screens;return 1;
}
int a7_gfx_create(struct a7_gfx_context*c){
 const struct a7_gfx_api*a=&c->api;struct a7_gfx_report*r=&c->report;r->create_tid=tid();
 if(r->create_tid!=r->owner_tid){r->thread_errors++;r->status=20;return 0;}
 if(!r->pre_valid||r->stage||!ids(a)||!a7_gfx_snapshot(a,&c->work)||!absent(&c->work)||!reserved_layers(a,&c->work)){r->status=22;return 0;}
 /* Entry uses its own fresh baseline; no assumption that registry order stayed
    unchanged while the original open Event was waiting in the event queue. */
 c->before=c->work;r->before_layers=c->before.layers;r->before_screens=c->before.screens;
 for(unsigned i=0;i<A7_GFX_LABELS;i++){c->labels[i]=a->allocate(0x370);if(!c->labels[i]){r->status=23;return 0;}}c->label=c->labels[0];r->label=(uint32_t)(uintptr_t)c->label;r->stage=1;
 a->record_ctor(c->record);c->record_live=1;a->font_ctor(c->font);c->font_live=1;
 const uint8_t white[4]={255,255,255,255};a->font_resource(c->font,0x02066003u);a->font_size(c->font,24);a->font_color(c->font,white);
 if(!a->font_create((void*)(uintptr_t)c->font[0],c->record)||(r->font_id=a->record_id(c->record))==0xffffffffu){r->status=24;return 0;}
 a->font_dtor(c->font);c->font_live=0;r->stage=2;
 const uint32_t layer_id=A7_LAYER_ID;
 c->layer=a->layer_create((void*)(uintptr_t)c->before.widget,&layer_id);
 if(!c->layer){r->status=25;return 0;}c->layer_live=1;r->layer=(uint32_t)(uintptr_t)c->layer;r->stage=3;
 a->layer_active(c->layer,0);r->screen=((uint32_t*)c->layer)[13];
 if(!r->screen||((uint32_t*)c->layer)[1]!=layer_id){r->status=26;r->uncertain=1;return 0;}
 static const char*const initial[A7_GFX_LABELS]={"SD 快门音","> 关闭","  自定义音效 1","  自定义音效 2","  自定义音效 3","  重新加载 SD 卡","  相机提示音","正在检查 SD 卡"};
 for(unsigned i=0;i<A7_GFX_LABELS;i++){
  void*p=c->labels[i];a->label_ctor(p);c->label_live++;r->stage=4;
  a->label_layer(p,&layer_id);float pos[2]={48.0f,i==0?36.0f:i==7?412.0f:64.0f+48.0f*(float)i};
  a->label_position(p,pos);a->label_size(p,544.0f,48.0f);a->label_font(p,r->font_id);
  unsigned k=0;while(initial[i][k]&&k<63){c->text[i][k]=initial[i][k];k++;}c->text[i][k]=0;a->label_text(p,c->text[i]);
 }r->stage=5;r->constructed=1;
 a->layer_active(c->layer,1);r->active=1;r->stage=6;return 1;
}
void a7_gfx_observe(struct a7_gfx_context*c){
 struct a7_gfx_report*r=&c->report;if(tid()!=r->owner_tid){r->thread_errors++;r->status=20;return;}
 r->dirty=0;for(unsigned i=0;i<c->label_live;i++)r->dirty|=((uint8_t*)c->labels[i])[0x2d5];
 int ok=a7_gfx_snapshot(&c->api,&c->work);r->reads=c->work.reads;r->bytes=c->work.bytes;
 if(!ok){r->status=27;return;}
 unsigned layers=0,screens=0;
 for(unsigned i=0;i<c->work.layers;i++)if(c->work.layer[i].id==A7_LAYER_ID&&c->work.layer[i].object==r->layer)layers++;
 for(unsigned i=0;i<c->work.screens;i++)if(c->work.screen[i].id==A7_LAYER_ID&&c->work.screen[i].object==r->screen)screens++;
 r->post_valid=layers==1&&screens==1&&reserved_layers(&c->api,&c->work);
 if(!r->post_valid)r->status=28;
}
int a7_gfx_destroy(struct a7_gfx_context*c){
 const struct a7_gfx_api*a=&c->api;struct a7_gfx_report*r=&c->report;r->destroy_tid=tid();
 if(r->destroy_tid!=r->owner_tid){r->thread_errors++;r->status=20;r->uncertain=1;return 0;}
 if(r->uncertain)return 0;
 /* A long-lived page can coexist with legitimate native Layer changes. Take
  * a fresh UI-owner snapshot immediately before destruction. Verify all other
  * entries are identical after removing only this page's exact Layer/Screen. */
 if(c->layer_live){
  if(!a7_gfx_snapshot(a,&c->pre_destroy)){r->status=31;r->uncertain=1;return 0;}
  unsigned ownl=0,owns=0;for(unsigned i=0;i<c->pre_destroy.layers;i++)if(c->pre_destroy.layer[i].id==A7_LAYER_ID){if(c->pre_destroy.layer[i].object!=r->layer){r->uncertain=1;r->status=32;return 0;}ownl++;}
  for(unsigned i=0;i<c->pre_destroy.screens;i++)if(c->pre_destroy.screen[i].id==A7_LAYER_ID){if(c->pre_destroy.screen[i].object!=r->screen){r->uncertain=1;r->status=32;return 0;}owns++;}
  if(ownl!=1||owns!=1){r->uncertain=1;r->status=32;return 0;}
  struct a7_gfx_snapshot*q=&c->pre_destroy;
  for(unsigned i=0;i<q->layers;i++)if(q->layer[i].id==A7_LAYER_ID){for(unsigned j=i+1;j<q->layers;j++)q->layer[j-1]=q->layer[j];q->layers--;break;}
  for(unsigned i=0;i<q->screens;i++)if(q->screen[i].id==A7_LAYER_ID){for(unsigned j=i+1;j<q->screens;j++)q->screen[j-1]=q->screen[j];q->screens--;break;}
 }else{if(!a7_gfx_snapshot(a,&c->pre_destroy)||!absent(&c->pre_destroy)){r->status=31;r->uncertain=1;return 0;}}
 if(c->layer_live){a->layer_active(c->layer,0);r->active=0;}
 while(c->label_live){unsigned i=c->label_live-1;a->label_dtor(c->labels[i]);c->label_live--;}
 for(unsigned i=0;i<A7_GFX_LABELS;i++)if(c->labels[i]){a->release(c->labels[i]);c->labels[i]=0;}c->label=0;
 if(c->font_live){a->font_dtor(c->font);c->font_live=0;}
 if(c->record_live){a->record_dtor(c->record);c->record_live=0;}
 if(c->layer_live){a->layer_dtor(c->layer);c->layer_live=0;a->release(c->layer);c->layer=0;}
 if(!a7_gfx_snapshot(a,&c->work)){r->status=29;r->uncertain=1;return 0;}
 r->after_layers=c->work.layers;r->after_screens=c->work.screens;r->restored=absent(&c->work)&&same_entries(&c->pre_destroy,&c->work);r->destructed=1;r->stage=7;
 if(!r->restored){r->status=30;r->uncertain=1;}return r->restored;
}

static int set_text(struct a7_gfx_context*c,unsigned label,const char*t){unsigned i=0;int changed=0;while(i<63&&t[i]){if(c->text[label][i]!=t[i])changed=1;i++;}if(t[i])return 0;if(c->text[label][i])changed=1;if(!changed)return 1;for(unsigned j=0;j<=i;j++)c->text[label][j]=t[j];c->api.label_text(c->labels[label],c->text[label]);return 1;}
int a7_gfx_update(struct a7_gfx_context*c,uint32_t row,uint32_t selected,uint32_t state){
 if(tid()!=c->report.owner_tid||c->label_live!=A7_GFX_LABELS||!c->report.constructed||c->report.uncertain)return 0;
 static const char*const names[6]={"关闭","自定义音效 1","自定义音效 2","自定义音效 3","重新加载 SD 卡","相机提示音"};
 for(unsigned i=0;i<6;i++){char line[64];line[0]=i==row?'>':' ';line[1]=' ';unsigned j=0;do{line[j+2]=names[i][j];}while(names[i][j++]&&j<60);if(!set_text(c,i+1,line))return 0;}
 const char*status=state==1?"正在应用":state==2?"SD就绪，联动未开启":state==3?"SD不可用":state==4?"请求失败":state==5?"已关闭":state==6?"快门音已开启":state==7?"音效处理中":state==8?"播放完成":state==9?"联动异常":state==10?"本次未播放":state==11?"音效3未配置":"正在检查SD";
 char line[64];unsigned p=0;const char*shortnames[3]={"关闭","音效1","音效2"};const char*name=selected<3?shortnames[selected]:"未知";const char*sep=" | ";
 /* Fixed reviewed strings fit as whole UTF-8 sequences. Reject overflow,
  * rather than truncating into a partial multibyte character. */
 while(*name){if(p==63)return 0;line[p++]=*name++;}while(*sep){if(p==63)return 0;line[p++]=*sep++;}while(*status){if(p==63)return 0;line[p++]=*status++;}line[p]=0;return set_text(c,7,line);
}
