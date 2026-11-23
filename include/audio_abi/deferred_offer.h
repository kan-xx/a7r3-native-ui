#ifndef DEFERRED_OFFER_H
#define DEFERRED_OFFER_H
#include <stdint.h>
enum {DF_EMPTY,DF_WAIT,DF_RELEASED,DF_CANCELLED,DF_EXPIRED,DF_FAULT};
struct df_status {uint32_t version,bytes,phase,control,asset,serial,desired,started_ms,elapsed_ms,checks,reason,max_ms;};
int df_start(uint32_t control,uint32_t asset,uint32_t word,uint32_t desired,uint32_t now_ms);
int df_tick(uint32_t now_ms,uint32_t word,uint32_t desired,int source_consumed,int owner_idle,int policy_result);
int df_cancel(uint32_t reason);
int df_pending(void);
void df_report(struct df_status*);
#endif
