#include "admission.h"
static uint32_t ld(const uint32_t*p){return __atomic_load_n(p,__ATOMIC_ACQUIRE);}
static int cas(uint32_t*p,uint32_t*e,uint32_t v){return __atomic_compare_exchange_n(p,e,v,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE);}
void adm_init(struct adm_gate*g){g->word=5;g->fatal=g->reason=g->revokes=g->commits=g->rejected=0;}
uint32_t adm_word(const struct adm_gate*g){return ld(&g->word);}
int adm_revoke(struct adm_gate*g,uint32_t reason){
 if(!g||!reason)return-1;__atomic_store_n(&g->reason,reason,__ATOMIC_RELEASE);
 for(unsigned i=0;i<4;i++){uint32_t old=ld(&g->word);if(old>=0xfffffff8u)break;if(cas(&g->word,&old,(old+4u)|ADM_CLOSED)){__atomic_fetch_add(&g->revokes,1,__ATOMIC_RELAXED);return 0;}}
 __atomic_store_n(&g->fatal,1,__ATOMIC_RELEASE);__atomic_fetch_or(&g->word,ADM_CLOSED,__ATOMIC_ACQ_REL);return-1;
}
int adm_open(struct adm_gate*g,uint32_t expected){if(!g||ld(&g->fatal)||!(expected&ADM_CLOSED)||(expected&ADM_COMMITTED))return-1;if(!cas(&g->word,&expected,expected&~ADM_CLOSED))return-1;if(ld(&g->fatal)){__atomic_fetch_or(&g->word,ADM_CLOSED,__ATOMIC_ACQ_REL);return-1;}return 0;}
int adm_commit(struct adm_gate*g,uint32_t expected){if(!g||ld(&g->fatal)||(expected&3u))return-1;if(cas(&g->word,&expected,expected|ADM_COMMITTED)){__atomic_fetch_add(&g->commits,1,__ATOMIC_RELAXED);return 0;}__atomic_fetch_add(&g->rejected,1,__ATOMIC_RELAXED);return-1;}
void adm_sent(struct adm_gate*g){__atomic_fetch_and(&g->word,~ADM_COMMITTED,__ATOMIC_RELEASE);}
