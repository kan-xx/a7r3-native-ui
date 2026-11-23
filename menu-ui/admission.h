#ifndef AB2_ADMISSION_H
#define AB2_ADMISSION_H
#include <stdint.h>
#define ADM_CLOSED 1u
#define ADM_COMMITTED 2u
struct adm_gate {uint32_t word,fatal,reason,revokes,commits,rejected;};
void adm_init(struct adm_gate*);
/* Bounded, allocation-free, callback-safe. Serial is local, not media epoch. */
int adm_revoke(struct adm_gate*,uint32_t reason);
uint32_t adm_word(const struct adm_gate*);
int adm_open(struct adm_gate*,uint32_t exact_closed_word);
int adm_commit(struct adm_gate*,uint32_t exact_open_word);
void adm_sent(struct adm_gate*);
#endif
