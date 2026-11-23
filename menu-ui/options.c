#include "options.h"
static uint32_t key(uint32_t e){switch(e&~15u){case 0x15010070:return 1;case 0x15010080:return 2;case 0x15010060:return 3;case 0x15010110:return 4;case 0x15010090:return 5;case 0x150100a0:return 6;default:return 0;}}
int mp_options_owns(uint32_t e){unsigned state=e&15;return key(e)&&(state==1||state==2||state==4);}
void mp_options_open(struct mp_options*s,uint32_t desired){for(unsigned i=0;i<sizeof(*s);i++)((uint8_t*)s)[i]=0;s->active=1;s->row=desired<3?desired:0;}
struct mp_action mp_options_event(struct mp_options*s,uint32_t e){struct mp_action a={0,s->row};if(!s->active||!mp_options_owns(e)){s->ignored++;return a;}unsigned k=key(e),state=e&15;s->last_event=e;
 /* Left/right are consumed, so they cannot change the underlying AA selection.
  * Other native input classes require final modal-routing qualification. */
 if(k>=5||state==4){s->ignored++;return a;}
 if(state==2){if(s->held||(s->pending&&k==3)){s->ignored++;return a;}s->held=k;if(k==1){s->row=(s->row+5)%6;a.action=MP_REDRAW;}else if(k==2){s->row=(s->row+1)%6;a.action=MP_REDRAW;}a.row=s->row;return a;}
 if(s->held!=k){s->ignored++;return a;}s->held=0;
 if(k==4){s->active=0;a.action=MP_BACK;return a;}
 if(k==3&&!s->pending){a.row=s->row;a.action=s->row<3?MP_CHOOSE:s->row==3?MP_EMPTY:s->row==4?MP_REFRESH:MP_STOCK_AUDIO;if(a.action==MP_CHOOSE||a.action==MP_REFRESH||a.action==MP_EMPTY)s->pending=1;}
 return a;
}
