#ifndef A7R_RESOURCE_DIAG_H
#define A7R_RESOURCE_DIAG_H
#include <stdint.h>
enum { RD_TABLE_IO=1, RD_TABLE_UNSTABLE=2, RD_TABLE_HEADER=3,
 RD_TABLE_BINDING=4, RD_PROC_IO=5, RD_PROC_DUPLICATE=6, RD_PROC_OVERFLOW=7,
 RD_PROC_CONTRACT=8, RD_PROC_BINDING=9, RD_PROC_PARSE=10, RD_PROC_MISSING=11 };
struct fr_diagnostic {uint32_t abi,bytes,reason,index,expected,observed;};
/* Copy only while serialized by the existing audio owner lock. */
void fr_diag_read(struct fr_diagnostic*);
/* Captured by the audio worker before any wake mutation. Survives a later
 * feature disable and is copied into the protected startup failure record. */
struct fr_domain_record {
 uint32_t abi,bytes,epoch,stage,result,mode[2],rebuilds;
 uint32_t table[91],stock[7][10],owned[2][10];
 uint32_t certified_epoch,closed_mask,rearm_index,rearm_rc;
};
_Static_assert(sizeof(struct fr_domain_record)==772,"domain record ABI");
void fr_domain_read(struct fr_domain_record*);
#endif
