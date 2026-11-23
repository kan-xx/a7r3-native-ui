#ifndef MP_PRODUCT_H
#define MP_PRODUCT_H
#include "native_port.h"
#include "audio_native.h"
#include "stock_open.h"
#include "diagnostic.h"
#define MP_BUILD 0x3110d015u
#define MP_SYMBOL "a7r_menu_product_v21"
struct mp_bind {uint32_t version,bytes,owner_tid,backend_kind;uint8_t audio_instance[16],activation_nonce[16];};
enum{MP_COLD,MP_INSTALLING,MP_RESIDENT,MP_REMOVING,MP_STOPPED_PINNED,MP_HOLD_PINNED};
struct mp_report{uint32_t version,bytes,phase,error,pid,owner,reference,callbacks,queued,registry_added,registry_removed,registry_node,factory_calls,factory_rejected,factory_lr,object,vptr,view_live,records_allocated,records_freed,backend_guard,callback_identity,stop_requested,backend_kind;
 struct mp_native_report native;struct mpr_report route;struct mp_page_report page;struct mat_report audio;struct a7_gfx_report graphics;struct a7_lean_report view;struct mp_stock_result stock;struct md_report diagnostic;uint8_t nonce[16];};
struct mp_api{uint32_t version,bytes,build;int(*start)(void*,const struct mp_bind*,uint32_t*,struct mp_report*);int(*query)(struct mp_report*);int(*request_remove)(struct mp_report*);int(*drain)(struct mp_report*);int(*diagnostic)(uint32_t,struct mp_report*);int(*snapshot)(uint32_t,uint32_t,uint8_t*,uint32_t,uint32_t*);};
_Static_assert(sizeof(struct mp_bind)==48,"bind ABI");
_Static_assert(sizeof(struct mp_report)==1148,"report ABI");
#endif
