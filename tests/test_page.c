#include "page_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,draws,backs,stocks,last_draw_status;static uint32_t current_tid=392;static void ok(int x){if(!x){fprintf(stderr,"FAIL %u\n",checks);exit(1);}checks++;}
static uint32_t tid(void*p){(void)p;return current_tid;}
static struct mat_observation state={.valid=1,.phase=2,.control=7,.desired=0,.owner_phase=1,.generation=8,.gate=1};
static int audio(void*p,const struct mat_request*q,struct mat_observation*out){(void)p;if(q->op==5)state.gate|=1;else if(q->op==6){ok((state.gate&3)==1);state.control=q->control;state.desired=q->asset==1?state.desired:q->asset?q->asset-1:0;state.generation++;state.qualified=1;state.gate=0;memcpy(state.snapshot_nonce,q->nonce,16);}else ok(q->op==2);*out=state;return q->op==2?0:1;}
static int draw(void*p,uint32_t row,uint32_t selected,uint32_t status){(void)p;ok(row<6);ok(selected<=2||selected==0xffffffffu);ok(status<=11);last_draw_status=status;draws++;return 1;}
static int back(void*p){(void)p;backs++;return 1;}static int stock(void*p){(void)p;stocks++;return 1;}
static void key(struct mp_page*p,uint32_t base){ok(mp_page_event(p,p->report.serial,base+2));ok(mp_page_event(p,p->report.serial,base+1));}
int main(void){struct mat_context a;struct mat_port ap={0,tid,audio};uint8_t seed[16]={3,2,1};ok(!mat_init(&a,&ap,392,seed));struct mp_page p;struct mp_page_port pp={0,tid,draw,back,stock};ok(mp_page_init(&p,&pp,&a,392));
 for(unsigned cycle=0;cycle<100;cycle++){ok(mp_page_open(&p));ok(!a.report.query_fresh);unsigned serial=p.report.serial;ok(mp_page_tick(&p));while(p.report.options.row!=1)key(&p,0x15010080);key(&p,0x15010060);ok(p.report.options.pending);key(&p,0x15010110);ok(backs==cycle+1);ok(mp_page_exit(&p,serial));ok(mp_page_needs_tick(&p));for(unsigned i=0;i<8&&mp_page_needs_tick(&p);i++)ok(mp_page_tick(&p));ok(a.report.completed&&state.desired==1&&!mp_page_needs_tick(&p));ok(!mp_page_event(&p,serial,0x15010062));}
 ok(mp_page_open(&p));unsigned old=p.report.serial;while(p.report.options.row!=5)key(&p,0x15010080);key(&p,0x15010060);ok(stocks==1&&p.report.original_queued);ok(mp_page_exit(&p,old));ok(mp_page_open(&p));ok(!mp_page_event(&p,old,0x15010062));key(&p,0x15010090);key(&p,0x150100a0);ok(p.report.options.row==0);current_tid=999;ok(!mp_page_tick(&p));current_tid=392;ok(mp_page_exit(&p,p.report.serial));ok(p.report.opens==102&&p.report.exits==102);/* Fresh capture tail + atomic gate dominate stale runtime automatic. */
 ok(mp_page_open(&p));state.desired=1;state.qualified=1;state.gate=4;state.automatic=1;state.capture=(struct mat_capture){.version=1,.bytes=128,.phase=2,.bound=1,.held_reference=1,.source_generation=9,.arm_control=state.control,.armed_serial=1};
 ok(mp_page_tick(&p)&&last_draw_status==6);state.gate=5;ok(mp_page_tick(&p)&&last_draw_status==5);
 state.gate=6;ok(mp_page_tick(&p)&&last_draw_status==7);state.gate=8;ok(mp_page_tick(&p)&&last_draw_status==2);
 state.gate=4;state.capture.arm_control--;ok(mp_page_tick(&p)&&last_draw_status==2);state.capture.arm_control=state.control;
 state.capture.phase=3;state.automatic=0;state.capture.audio_phase=2;state.capture.audio_result=0;ok(mp_page_tick(&p)&&last_draw_status==8);
 state.capture.audio_phase=5;ok(mp_page_tick(&p)&&last_draw_status==7);state.capture.audio_phase=3;ok(mp_page_tick(&p)&&last_draw_status==10);state.capture.phase=4;state.capture.error=7;ok(mp_page_tick(&p)&&last_draw_status==9);
 state.gate=5;ok(mp_page_tick(&p)&&last_draw_status==5);state.fatal=1;ok(mp_page_tick(&p)&&last_draw_status==0&&!a.report.query_fresh);state.fatal=0;state.supervisor_error=4;ok(mp_page_tick(&p)&&last_draw_status==0&&!a.report.query_fresh);state.supervisor_error=0;ok(mp_page_exit(&p,p.report.serial));
 /* Reserved slot 3 is a real selectable row, but never sends invalid asset4. */
 ok(mp_page_open(&p));state.desired=2;state.qualified=1;state.gate=4;
 ok(mp_page_tick(&p));while(p.report.options.row!=3)key(&p,0x15010080);
 unsigned prior=p.report.requests;key(&p,0x15010060);
 ok(p.empty_slot&&a.report.asset==0&&p.report.options.pending);
 for(unsigned i=0;i<10&&p.report.options.pending;i++)ok(mp_page_tick(&p));
 ok(state.desired==0&&p.report.requests==prior+1&&last_draw_status==11);
 key(&p,0x15010080);ok(p.report.options.row==4);key(&p,0x15010060);
 ok(!p.empty_slot&&a.report.asset==1);for(unsigned i=0;i<10&&p.report.options.pending;i++)ok(mp_page_tick(&p));
 key(&p,0x15010080);ok(p.report.options.row==5);key(&p,0x15010080);ok(p.report.options.row==0);
 key(&p,0x15010070);ok(p.report.options.row==5);ok(mp_page_exit(&p,p.report.serial));
 struct lp_control power={0};p.power=&power;ok(mp_page_open(&p));
 unsigned events=p.report.event_count,row=p.report.options.row,seq=a.report.request_seq;
 ok(lp_pause(&power,1)==LP_OK);ok(!mp_page_event(&p,p.report.serial,0x15010082));
 ok(p.report.event_count==events&&p.report.options.row==row&&a.report.request_seq==seq);
 ok(lp_park(&power,1)==LP_OK);ok(lp_resume(&power,1)==LP_OK);ok(!mp_page_event(&p,p.report.serial,0x15010082));
 ok(lp_open(&power,1)==LP_OK);key(&p,0x15010080);ok(p.report.event_count==events+2&&p.report.options.row!=row);
 ok(mp_page_exit(&p,p.report.serial));p.power=0;
 printf("PAGE tests passed %u assertions; reserved empty slot disables playback; audio completes after Back; paused input closed\n",checks);return 0;}
