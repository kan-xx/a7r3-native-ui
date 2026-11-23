#ifndef AP_F4_SOURCE_H
#define AP_F4_SOURCE_H
#include <stdint.h>
#define AP_F4_COPY_BYTES 22592u
/* Immutable verified copy for an explicit manual trial. This is NOT a media
 * lease, playback admission, exposure fence, or a live refresh subscription. */
struct ap_f4_source_meta {
 uint32_t abi,bytes,status,stage,query_calls,audit_calls,copied,close_uncertain,runtime_base,runtime_build;
 uint8_t instance[16];uint32_t generation,control;
 uint8_t snapshot_nonce[16],media[24],sha256[32],asset_sha256[2][32];
 uint32_t source_is_lease,native_rc;
};
_Static_assert(sizeof(struct ap_f4_source_meta)==208,"F4 source metadata ABI");
enum ap_f4_copy_rc {AP_F4_OK=0,AP_F4_ARGUMENT=1,AP_F4_FILE=2,AP_F4_BIND=3,AP_F4_ABI=4,AP_F4_QUERY=5,AP_F4_AUDIT=6,AP_F4_STALE=7,AP_F4_HASH=8,AP_F4_CLOSE=9};
int ap_f4_copy(const uint8_t instance[16],uint32_t expected_generation,uint32_t expected_control,const uint8_t expected_snapshot_nonce[16],uint8_t out[AP_F4_COPY_BYTES],struct ap_f4_source_meta *meta);
#endif
