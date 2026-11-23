#include "diagnostic.h"
static int eq(const char*a,const char*b){for(unsigned i=0;i<96;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;}
static int snap(struct md_context*c,unsigned index){if(!c->port.native->refresh()||!c->port.snapshot(c->port.context,&c->snap[index]))return 0;c->report.snapshots|=1u<<index;return 1;}
static unsigned count(const struct a7_menu_snapshot*s,uint32_t id){unsigned n=0;for(unsigned i=0;i<s->active_count;i++)n+=s->active[i].id==id;return n;}
static int aa(struct md_context*c,const struct a7_menu_snapshot*s,int initial){uint32_t obj=0,vp=0,n=0,bits=0;for(unsigned i=0;i<s->active_count;i++){const struct a7_menu_view*v=&s->active[i];if(v->id!=0xaa)continue;if(v->index!=19||v->bank>1||!v->view||!v->registry_matches||!eq(v->factory,"ViewSettingMenuToInstance")||!eq(v->module,"viewUnified2.so")||(n&&(obj!=v->view||vp!=v->vptr)))return 0;obj=v->view;vp=v->vptr;bits|=1u<<v->bank;n++;}if(n!=2||bits!=3)return 0;if(initial){c->report.aa=obj;c->report.aa_vptr=vp;return 1;}return c->report.aa==obj&&c->report.aa_vptr==vp;}
static int stock(struct md_context*c,const struct a7_menu_snapshot*s){uint32_t base=0,func=0,bid=0,fid=0,bn=0,fn=0,bb=0,fb=0;for(unsigned i=0;i<s->active_count;i++){const struct a7_menu_view*v=&s->active[i];int b=eq(v->factory,"ViewCommonDedicatedScreenBaseToInstance"),f=eq(v->factory,"ViewCommonDedicatedScreenFuncToInstance");if(!b&&!f)continue;if((b&&v->id!=25)||(f&&v->id!=26)||v->index!=24)return 0;if(!v->registry_matches||!eq(v->module,"viewUnified4.so")||v->bank>1||!v->view)return 0;if(b){if(bn&&(base!=v->view||bid!=v->id))return 0;base=v->view;bid=v->id;bn++;bb|=1u<<v->bank;}else{if(fn&&(func!=v->view||fid!=v->id))return 0;func=v->view;fid=v->id;fn++;fb|=1u<<v->bank;}}
 if((bn&&(bn!=2||bb!=3))||(fn&&(fn!=2||fb!=3)))return 0;c->report.stock_base=base;c->report.stock_func=func;c->report.stock_base_id=bid;c->report.stock_func_id=fid;return 1;}
static void fail(struct md_context*c,uint32_t code){if(!c->report.error)c->report.error=code;c->report.stage=MD_CLEANUP;c->close_sent=0;c->wait_ticks=0;}
static int nav(struct md_context*c,uint32_t closing,uint32_t id){struct a7_navigation_result r={0};a7_navigation_target(&c->port.native->navigation,closing,id,0,&r);if(r.caught_exception){c->report.event_exceptions++;return 0;}return r.submitted;}
static int emit(struct md_context*c,uint32_t id,int own){
 if(!snap(c,3)||!aa(c,&c->snap[3],0))return 0;if(own&&!c->port.own_live(c->port.context))return 0;
 uint32_t top_index=0,top_id=0,top_obj=0;for(unsigned i=0;i<c->snap[3].active_count;i++){const struct a7_menu_view*v=&c->snap[3].active[i];if(v->bank==0&&v->index>=top_index){top_index=v->index;top_id=v->id;top_obj=v->view;}}
 if(own){if(top_id!=0xffc||top_index!=24)return 0;}else{if(!stock(c,&c->snap[3])||!c->report.stock_func||top_id!=c->report.stock_func_id||top_obj!=c->report.stock_func)return 0;}
 return md_inject(c->port.native,c->snap[3].manager,id,&c->report);
}
int md_start(struct md_context*c,const struct md_port*p){if(!c||!p||!p->native||!p->page||!p->owner||!p->snapshot||!p->own_live||!p->close_own||!p->entry_context||!p->resident_literal||c->attempted)return 0;c->attempted=1;c->port=*p;c->report.version=1;c->report.stage=MD_BASELINE;c->report.requests_before=p->page->report.requests;c->report.sequence_base=p->page->audio->last_seq;return 1;}
void md_continue(struct md_context*c){if(c&&(c->report.stage==MD_PAUSE_OWN||c->report.stage==MD_PAUSE_STOCK))c->continue_requested=1;}
int md_needs_tick(const struct md_context*c){return c&&c->attempted&&c->report.stage!=MD_DONE&&c->report.stage!=MD_HOLD;}
static int move_and_enter(struct md_context*c,uint32_t row){
 if(c->key_base){uint32_t key=c->key_base;c->key_base=0;if(!emit(c,key+1,1))return -1;return key==0x15010060?1:0;}
 c->key_base=c->port.page->report.options.row==row?0x15010060:0x15010080;if(!emit(c,c->key_base+2,1)){c->key_base=0;return -1;}return 0;
}
static void cleanup(struct md_context*c){
 if(!c->report.aa_owned){c->report.stage=c->report.event_exceptions?MD_HOLD:MD_DONE;c->report.restored=!c->report.event_exceptions;return;}
 if(++c->wait_ticks>80){c->report.stage=MD_HOLD;return;}
 if(!snap(c,3)){c->report.stage=MD_HOLD;return;}
 if(c->port.own_live(c->port.context)){if(!c->close_sent){if(!c->port.close_own(c->port.context)){c->report.stage=MD_HOLD;return;}c->close_sent=1;}return;}
 if(!stock(c,&c->snap[3])){c->report.stage=MD_HOLD;return;}
 if(c->report.stock_func||c->report.stock_base){uint32_t id=c->report.stock_func?c->report.stock_func_id:c->report.stock_base_id;if(c->close_sent!=id){if(!nav(c,1,id)){c->report.stage=MD_HOLD;return;}c->close_sent=id;}return;}
 if(count(&c->snap[3],0xaa)){
  /* Abort may arrive after our accepted AA open but before MD_AA_OPEN has
   * captured its identity. Perform that same bounded identity check without
   * executing the menu action; no route/selection change has begun yet. */
  if(!c->report.aa&&!c->report.aa_vptr&&!c->route.dirty&&!c->route.report.action_called){if(!aa(c,&c->snap[3],1)){c->report.stage=MD_HOLD;return;}}
  if(!c->report.aa_owned||!aa(c,&c->snap[3],0)||!a7_route_restore(&c->route)){c->report.stage=MD_HOLD;return;}
  if(c->close_sent!=0xaa){if(!nav(c,1,0xaa)){c->report.stage=MD_HOLD;return;}c->close_sent=0xaa;}return;
 }
 if(c->route.report.action_called&&!a7_route_finish_flag(&c->route)){c->report.stage=MD_HOLD;return;}
 c->report.restored=1;c->report.final_aa_count=0;c->report.stage=MD_DONE;
}
int md_tick(struct md_context*c){
 if(!md_needs_tick(c))return 1;c->report.ticks++;
 if(c->report.stage==MD_PAUSE_OWN||c->report.stage==MD_PAUSE_STOCK){if(++c->pause_ticks>1200)fail(c,100);}
 else{c->pause_ticks=0;if(c->report.stage!=MD_CLEANUP&&++c->active_ticks>600)fail(c,100);}
 struct mp_page*p=c->port.page;struct mp_native_api*n=c->port.native;
 switch(c->report.stage){
 case MD_BASELINE:{uint8_t flag=0;if(!snap(c,0)||count(&c->snap[0],0xaa)||count(&c->snap[0],0xffc)||!stock(c,&c->snap[0])||c->report.stock_base||c->report.stock_func||!n->reader(0,n->setting_status_byte,&flag,1)||flag>1){fail(c,101);break;}c->report.baseline_flag=flag;if(!nav(c,0,0xaa)){fail(c,102);break;}c->report.aa_owned=1;c->report.stage=MD_AA_OPEN;c->wait_ticks=0;break;}
 case MD_AA_OPEN:if(!snap(c,3)){fail(c,103);break;}if(!aa(c,&c->snap[3],1)){if(++c->wait_ticks>20)fail(c,104);break;}{struct a7_route_api a=n->test_route;a.original_target=c->port.resident_literal;a.persistent_route=1;if(!a7_route_prepare(&c->route,&a,c->report.aa,c->report.aa_vptr)||!a7_route_execute(&c->route)){fail(c,105);break;}c->report.stage=MD_OWN_OPEN;c->wait_ticks=0;}break;
 case MD_OWN_OPEN:case MD_REOPEN:if(!c->port.own_live(c->port.context)||!p->report.active){if(++c->wait_ticks>20)fail(c,106);break;}c->report.opens++;c->wait_ticks=0;c->key_base=0;c->report.stage=c->report.stage==MD_REOPEN?MD_STOCK_SELECT:MD_SCRIPT;break;
 case MD_SCRIPT:if(c->target_pending){if(!p->report.options.pending){if(!p->audio->report.completed||p->audio->report.request_seq!=c->request_expected){fail(c,107);break;}struct mat_report*m=&p->audio->report;struct md_receipt*r=&c->report.receipts[c->target_index];*r=(struct md_receipt){m->request_seq,m->asset,m->target,m->accepted,m->completed,m->code,m->control,m->old_generation,m->new_generation,m->qualified,m->gate,m->automatic,{0}};for(unsigned j=0;j<16;j++)r->nonce[j]=m->nonce[j];c->target_pending=0;c->target_index++;}break;}if(c->target_index>=4){if(!snap(c,1)){fail(c,108);break;}c->report.stage=MD_PAUSE_OWN;break;}{int r=move_and_enter(c,c->target_index);if(r<0){fail(c,109);break;}if(r==1){if(!p->report.options.pending||p->audio->report.request_seq<=c->request_expected){fail(c,110);break;}c->request_expected=p->audio->report.request_seq;c->target_pending=1;}}break;
 case MD_PAUSE_OWN:if(c->continue_requested){c->continue_requested=0;c->report.continued++;c->report.stage=MD_OWN_CLOSE;c->return_key=0;c->wait_ticks=0;}break;
 case MD_OWN_CLOSE:if(c->return_key<2){if(!emit(c,0x15010110+(c->return_key?1:2),1)){fail(c,111);break;}c->return_key++;}else if(!c->port.own_live(c->port.context)){if(!snap(c,3)||!aa(c,&c->snap[3],0)||!c->port.entry_context(c->port.context)){fail(c,112);break;}uint32_t ex=0;if(!a7_route_action_caught(n->test_route.action,(void*)(uintptr_t)c->report.aa,&ex)||ex){fail(c,113);break;}c->report.stage=MD_REOPEN;c->wait_ticks=0;}else if(++c->wait_ticks>20)fail(c,114);break;
 case MD_STOCK_SELECT:{int r=move_and_enter(c,5);if(r<0){fail(c,115);break;}if(r==1){if(!p->report.original_queued){fail(c,116);break;}c->report.stage=MD_STOCK_OPEN;c->wait_ticks=0;}}break;
 case MD_STOCK_OPEN:if(!snap(c,2)||!aa(c,&c->snap[2],0)||!stock(c,&c->snap[2])){fail(c,117);break;}if(!c->report.stock_base&&c->report.stock_func&&!c->port.own_live(c->port.context)){c->report.stage=MD_PAUSE_STOCK;}else if(++c->wait_ticks>20)fail(c,118);break;
 case MD_PAUSE_STOCK:if(c->continue_requested){c->continue_requested=0;c->report.continued++;c->report.stage=MD_STOCK_RETURN;c->return_key=0;c->wait_ticks=0;}break;
 /* Original stock MENU press may enqueue closure immediately. Both events
  * run in one owner callback, before the event loop consumes that queue. Each
  * emission freshly qualifies the same live stock top view; never deliver a
  * late release to whichever unrelated view became top on the next tick. */
 case MD_STOCK_RETURN:if(c->return_key<2){if(!emit(c,0x15010112,0)){fail(c,119);break;}c->return_key=1;if(!emit(c,0x15010111,0)){fail(c,119);break;}c->return_key=2;}else{if(!snap(c,3)||!aa(c,&c->snap[3],0)||!stock(c,&c->snap[3])){fail(c,120);break;}if(!c->report.stock_base&&!c->report.stock_func)c->report.stage=MD_RESTORE;else if(++c->wait_ticks>20)fail(c,121);}break;
 case MD_RESTORE:if(!a7_route_restore(&c->route)){fail(c,122);break;}if(!nav(c,1,0xaa)){fail(c,123);break;}c->report.stage=MD_AA_CLOSE;c->wait_ticks=0;break;
 case MD_AA_CLOSE:if(!snap(c,3)){fail(c,124);break;}c->report.final_aa_count=count(&c->snap[3],0xaa);if(!c->report.final_aa_count&&!count(&c->snap[3],0xffc)){if(!a7_route_finish_flag(&c->route)){fail(c,125);break;}c->report.restored=1;c->report.stage=MD_DONE;}else if(++c->wait_ticks>20)fail(c,126);break;
 case MD_CLEANUP:cleanup(c);break;default:fail(c,127);break;
 }
 c->report.route=c->route.report;return c->report.stage!=MD_HOLD;
}

void md_abort(struct md_context*c){if(c&&md_needs_tick(c)&&c->report.stage!=MD_CLEANUP)fail(c,130);}
