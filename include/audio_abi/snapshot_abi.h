#ifndef A7_SNAPSHOT_ABI_H
#define A7_SNAPSHOT_ABI_H
#include <stdint.h>
#include <stddef.h>
#define A7_SNAPSHOT_ABI 1u
#define A7_SNAPSHOT_ASSETS 2u
#define A7_SNAPSHOT_SLOT_CAP 25600u
#define A7_SNAPSHOT_BYTES (2u * A7_SNAPSHOT_SLOT_CAP)
#define A7_SNAPSHOT_CONTROLLED_DIAGNOSTIC 1u
#define A7_ASSET_MEOW 2u
#define A7_ASSET_FOCUS 3u
#define A7_FORMAT_NATIVE_KNOWN_FIXED 1u
struct a7_snapshot_asset_v1 {uint32_t asset_id,length,offset,format;uint8_t sha256[32];};
struct a7_snapshot_manifest_v1 {
 uint32_t abi,total_bytes,asset_count,qualification_flags;
 uint8_t media_id[24];uint32_t reserved[2];
 struct a7_snapshot_asset_v1 assets[2];
};
_Static_assert(sizeof(struct a7_snapshot_asset_v1)==48,"fixed descriptor ABI");
_Static_assert(sizeof(struct a7_snapshot_manifest_v1)==144,"fixed manifest ABI");
_Static_assert(offsetof(struct a7_snapshot_manifest_v1,assets)==48,"descriptor offset");
/* Cross-DSO import must copy manifest and bytes synchronously before returning;
   no borrowed buffers, path strings, callbacks or function pointers survive.
   qualification_flags=1 asserts CONTROLLED DIAGNOSTIC ONLY, never media epoch.
   Validating this ABI never grants playback eligibility or changes desired. */
/* Returns 0 valid AudioIP data & digest, 14 wrong data hash, 17 malformed manifest. */
int a7_snapshot_validate(const struct a7_snapshot_manifest_v1 *,const uint8_t *,size_t);
#endif
