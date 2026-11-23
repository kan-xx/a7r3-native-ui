#ifndef A7_MEDIA_SAMPLE_H
#define A7_MEDIA_SAMPLE_H
#include <stdint.h>
/* Borrowed original libInfraFsys entry points. Host pins and authenticates the
 * exact original provider until the only background worker has returned.
 * No API here opens a media/file handle or changes native media state.
 * Native calls may wait on original locks: never execute from UI or callbacks.
 * Input table/output must be live and nonoverlapping, with exclusive output
 * ownership. No callback or caller may free the provider/context while active. */
struct mr_fsys {
 uint32_t (*mounts)(void);
 uint16_t (*letter)(const char *);
 int32_t (*info)(uint32_t, void *);
};
enum mr_reason {MR_OK=0,MR_ARGUMENT=1,MR_MOUNT=2,MR_LETTER=3,MR_INFO=4,
 MR_STATUS=5,MR_FSTYPE=6,MR_CHANGED=7};
struct mr_round {
 uint32_t mounts,letter;
 int32_t info_result;
 uint32_t info_called;
 uint8_t info[56];
};
struct mr_sample {
 uint32_t abi,bytes,reason,available,qualification,native_calls;
 struct mr_round round[2];
};
_Static_assert(sizeof(struct mr_round)==72,"round ABI");
_Static_assert(sizeof(struct mr_sample)==168,"sample ABI");
/* Forward mounts/letter/info, then reverse info/letter/mounts. Success is only
 * a readable-state hint, not ownership, current-card identity or a lease.
 * info includes opaque 24 B. Equal values cannot detect remove/reinsert ABA.
 * Only the B mount bit matters; changes to an unrelated drive do not revoke B.
 * Nonzero status is conservatively unavailable, including read-only status.
 * Native errors do not permit reusing a retained old snapshot as current. */
int mr_sample(const struct mr_fsys *,struct mr_sample *);
/* Pure comparison. LOSS/CHANGED => cancel own refresh/use-admission immediately;
 * RECOVERED => may queue a NEW full hash/EOF/Close transaction on own worker.
 * UNCHANGED never reopens admission. Each input has qualification=0. No function
 * here changes F4 or grants playback. Caller atomically fences publication using
 * its own cancel serial; never use this result as a media epoch. */
enum mr_transition {MR_UNCHANGED=0,MR_LOSS=1,MR_DIFFERENT=2,MR_RECOVERED=3};
int mr_transition(const struct mr_sample *previous,const struct mr_sample *current);
#endif
