#ifndef CA_SOURCE_H
#define CA_SOURCE_H
#include <stdint.h>
#include "observer_f7_abi.h"
enum {CA_UNBOUND,CA_DISARMED,CA_ARMED,CA_CONSUMED,CA_FAULT};
struct ca_status {uint32_t version,bytes,phase,error,bound,base,entry,source_generation,source_pid,floor,last_order,arm_control,offer_control,armed_serial,observed,eligible,accepted,skipped,old,duplicates,busy,audio_phase,audio_result,dropped,bad_message,last_capture,last_sequence,last_media,last_tid,last_revoke,held_reference,source_matched;};
_Static_assert(sizeof(struct ca_status)==128,"auto status");
/* Single audio owner only; no event callback, original pin mutation or PLAY. */
int ca_bind(const uint8_t nonce[16],uint32_t generation);
int ca_arm(uint32_t control,uint32_t admission_word,uint32_t desired);
int ca_auto_arm(uint32_t control,uint32_t admission_word,uint32_t desired);
int ca_tick(uint32_t admission_word,int idle,uint32_t desired,uint32_t *asset,uint32_t *offer_control);
void ca_close(uint32_t reason);
void ca_sound_result(uint32_t phase,uint32_t result);
void ca_report(struct ca_status*);
#endif
