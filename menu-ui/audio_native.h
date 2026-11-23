#ifndef MENU_AUDIO_NATIVE_H
#define MENU_AUDIO_NATIVE_H
#include "audio_transaction.h"
struct mp_audio_native {void*guard;const void*api;uint32_t owner_tid,admission,ui_calls,closed,reference_uncertain,base,backend_kind;uint8_t instance[16];};
int mp_audio_host_bind(struct mp_audio_native*,uint32_t,uint32_t,const uint8_t[16],struct mat_port*);
int mp_audio_ui_close(struct mp_audio_native*);
/* External caller must have drained all its timer/View callbacks first. */
int mp_audio_host_release(struct mp_audio_native*);
int mp_audio_host_abort_unpublished(struct mp_audio_native*);
#endif
