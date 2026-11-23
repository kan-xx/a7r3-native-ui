#ifndef MP_DIAGNOSTIC_H
#define MP_DIAGNOSTIC_H
#include "native_port.h"
#include "page_control.h"
enum{MD_IDLE,MD_BASELINE,MD_AA_OPEN,MD_OWN_OPEN,MD_SCRIPT,MD_PAUSE_OWN,MD_OWN_CLOSE,MD_REOPEN,MD_STOCK_SELECT,MD_STOCK_OPEN,MD_PAUSE_STOCK,MD_STOCK_RETURN,MD_RESTORE,MD_AA_CLOSE,MD_DONE,MD_CLEANUP,MD_HOLD};
struct md_receipt{uint32_t sequence,asset,target,accepted,completed,code,control,old_generation,new_generation,qualified,gate,automatic;uint8_t nonce[16];};
struct md_report{uint32_t version,stage,error,ticks,opens,aa,aa_vptr,aa_owned,events,constructed,destroyed,event_exceptions,key_index,requests_before,sequence_base,continued,stock_base,stock_func,stock_base_id,stock_func_id,snapshots,restored,final_aa_count,baseline_flag;struct a7_route_report route;struct md_receipt receipts[4];};
struct md_port{void*context;struct mp_native_api*native;struct mp_page*page;uint32_t(*owner)(void*);int(*snapshot)(void*,struct a7_menu_snapshot*);int(*own_live)(void*);int(*close_own)(void*);int(*entry_context)(void*);uint32_t resident_literal;};
struct md_context{struct md_port port;struct md_report report;struct a7_route_context route;struct a7_menu_snapshot snap[4];uint32_t continue_requested,attempted,return_key,close_sent,wait_ticks,target_index,key_base,target_pending,request_expected,active_ticks,pause_ticks;};
int md_start(struct md_context*,const struct md_port*);
void md_continue(struct md_context*);void md_abort(struct md_context*);
int md_tick(struct md_context*);
int md_needs_tick(const struct md_context*);
#ifdef __cplusplus
extern "C" {
#endif
int md_inject(const struct mp_native_api*,uint32_t vm,uint32_t id,struct md_report*);
#ifdef __cplusplus
}
#endif
#endif
