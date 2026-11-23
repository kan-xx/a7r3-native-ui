#ifndef A7_OBSERVER_F7_ABI_H
#define A7_OBSERVER_F7_ABI_H
#include <stdint.h>
#include <stddef.h>
#define F7_ABI 1u
#define F7_BUILD 0x3110f702u
#define F7_CORE_PATH "/usr/lib/liba7r_exposure_observer_v2.so"
#define F7_CORE_SYMBOL "a7r_exposure_observer_v2"
enum {F7_EMPTY=0,F7_ARMED=1,F7_STOPPED_RETAINED=2,F7_ERROR_RETAINED=3};
enum {F7_OK=0,F7_BUSY=1,F7_BIND=2,F7_ONCE_ONLY=3,F7_CAS=4,F7_BAD_GENERATION=5,F7_CONFLICT=6};
typedef struct {uint32_t order,pid,tid,caller,pin,outer,category,message,present,capture_id,sequence_id,media,phase,generation,source_kind,qualification;} f7_record;
typedef struct {
 uint32_t abi,build,phase,result,generation,owns_reference,got_restored,ever_published;
 uint32_t pid,last_tid,active,records,seen,matched,dropped,bad_message;
 uint32_t obj_base,mwf_base,got_address,original_target,trampoline,current_got,bind_stage,bind_index;
 uint32_t bind_failure,provider_mask,provider_close_fail_mask,cas_observed,source_kind,qualification,providers_retained,flags;
 uint8_t instance_nonce[16];uint32_t reserved[28];
} f7_status;
typedef struct {
 uint32_t abi,size,build;
 int (*attach)(void*,const uint8_t nonce[16],uint32_t *adopted,f7_status*);
 int (*query)(f7_status*);
 int (*drain)(uint32_t expected,f7_record*,uint32_t capacity,uint32_t*count,f7_status*);
 int (*stop)(uint32_t expected,f7_status*);
} f7_api;
_Static_assert(sizeof(f7_record)==64,"record");_Static_assert(sizeof(f7_status)==256,"status");
#endif
