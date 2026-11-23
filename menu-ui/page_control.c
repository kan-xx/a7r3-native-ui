#include "page_control.h"
static int owner(struct mp_page*p){return p&&p->port.tid&&p->port.tid(p->port.context)==p->report.owner;}
static int terminal(const struct mat_context*c){return c->report.state==MAT_IDLE||c->report.state==MAT_DONE||c->report.state==MAT_FAILED;}
static int draw(struct mp_page*p){const struct mat_report*m=&p->audio->report;uint32_t selected=m->query_fresh?m->observed_desired:0xffffffffu,state=0;if(!terminal(p->audio))state=1;else if(m->state==MAT_FAILED)state=4;else if(m->query_fresh){
 const struct mat_capture*c=&m->capture;
 /* The atomically closed gate dominates an older automatic flag/tail. A
  * consumed window is never presented as armed or continuously enabled. */
 if(m->observed_desired==0||(m->gate&1))state=5;
 else if(c->version==1&&(c->error||c->phase==4))state=9;
 else if(m->qualified){
  state=(m->gate&2)?7:2;
  int window=c->version==1&&c->bytes==128&&c->bound&&c->held_reference&&c->source_generation&&c->arm_control&&c->arm_control==m->observed_control&&c->armed_serial==(m->gate>>2)&&!c->dropped&&!c->bad_message;
  if(window&&c->phase==2&&m->automatic&&!(m->gate&3))state=6;
  else if(window&&c->phase==3){if(c->audio_phase==1||c->audio_phase==5)state=7;else if(c->audio_phase==2&&!c->audio_result&&!(m->gate&3))state=8;else if(c->audio_phase==4)state=9;else if(!(m->gate&3))state=10;}
 }else state=3;
 }
 if(p->empty_slot&&p->report.options.row==3&&m->state==MAT_DONE&&m->query_fresh&&m->observed_desired==0)state=11;
 p->report.draws++;if(!p->port.draw(p->port.context,p->report.options.row,selected,state)){p->report.error=1;return 0;}return 1;
}
int mp_page_init(struct mp_page*p,const struct mp_page_port*port,struct mat_context*a,uint32_t owner_tid){if(!p||!port||!port->tid||!port->draw||!port->close_own||!port->open_stock_audio||!a||!owner_tid||a->report.owner_tid!=owner_tid)return 0;for(unsigned i=0;i<sizeof(*p);i++)((uint8_t*)p)[i]=0;p->port=*port;p->audio=a;p->report.version=1;p->report.owner=owner_tid;return 1;}
int mp_page_open(struct mp_page*p){if(!owner(p)||p->report.active||p->report.serial==0xffffffffu)return 0;p->report.serial++;p->report.opens++;p->report.active=1;p->report.close_queued=p->report.original_queued=p->report.error=0;
 /* A previous page's QUERY is not current after an arbitrarily long close.
  * First paint is unknown; the next bounded owner tick obtains fresh status. */
 p->empty_slot=0;p->audio->report.query_fresh=0;mp_options_open(&p->report.options,0xffffffffu);p->report.options.pending=!terminal(p->audio);return draw(p);}
static int page_event(struct mp_page*p,uint32_t serial,uint32_t event){if(!owner(p)||!p->report.active||serial!=p->report.serial||p->report.close_queued||p->report.original_queued||!mp_options_owns(event))return 0;p->report.event_count++;struct mp_action a=mp_options_event(&p->report.options,event);
 if(a.action==MP_CHOOSE||a.action==MP_REFRESH||a.action==MP_EMPTY){if(p->last_seq==0xffffffffu){p->report.options.pending=0;p->report.error=2;return draw(p);}/* An unconfigured slot disables custom playback; never map it to a different asset. */
 p->empty_slot=a.action==MP_EMPTY;uint32_t asset=a.action==MP_EMPTY?0:a.action==MP_REFRESH?1:a.row==0?0:a.row+1;int rc=mat_submit(p->audio,asset,++p->last_seq);if(rc){p->report.options.pending=0;p->report.error=(uint32_t)rc;}else p->report.requests++;return draw(p);}
 if(a.action==MP_BACK||a.action==MP_STOCK_AUDIO){int ok=a.action==MP_BACK?p->port.close_own(p->port.context):p->port.open_stock_audio(p->port.context);if(ok){p->report.options.active=0;if(a.action==MP_BACK)p->report.close_queued=1;else p->report.original_queued=1;}else{p->report.options.active=1;p->report.error=3;}return ok;}
 if(a.action==MP_REDRAW)return draw(p);return 1;
}
int mp_page_event(struct mp_page*p,uint32_t serial,uint32_t event){
 if(!owner(p))return 0;struct lp_control*g=p->power;
 if(g&&lp_commit(g,adm_word(&g->gate)))return 0;
 int r=page_event(p,serial,event);if(g)lp_sent(g);return r;
}
int mp_page_tick(struct mp_page*p){if(!owner(p))return 0;p->report.ticks++;int rc=terminal(p->audio)?p->report.active?mat_observe(p->audio):0:mat_tick(p->audio);if(rc!=0&&rc!=MAT_BUSY)p->report.error=(uint32_t)rc;if(p->report.active){p->report.options.pending=!terminal(p->audio);return draw(p);}return 1;}
int mp_page_exit(struct mp_page*p,uint32_t serial){if(!owner(p)||!p->report.active||serial!=p->report.serial)return 0;p->report.exits++;p->report.active=0;p->report.options.active=0;p->report.close_queued=p->report.original_queued=0;return 1;}
int mp_page_needs_tick(const struct mp_page*p){return p&&(p->report.active||!terminal(p->audio));}
