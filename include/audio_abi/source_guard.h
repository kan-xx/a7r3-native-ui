#ifndef AP_SOURCE_GUARD_H
#define AP_SOURCE_GUARD_H
#include "ap_f4_source.h"
#include "snapshot_abi.h"
int a7_source_init(const uint8_t[16],uint32_t,uint32_t,const uint8_t[16],const uint8_t[24]);
int a7_source_copy(unsigned,void*,unsigned);
const uint8_t*a7_source_bytes(void);
void a7_source_manifest(unsigned,struct a7_snapshot_manifest_v1*);
void a7_source_status(struct ap_f4_source_meta*);
#endif
