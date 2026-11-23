#ifndef A7_MENU_MAPS_H
#define A7_MENU_MAPS_H
#include <stdint.h>
#define A7_MENU_MAP_TEXT_MAX 262144u
#define A7_MENU_MAP_LINES_MAX 4096u
#define A7_MENU_MAP_RANGES_MAX 2048u
struct a7_menu_range {uint32_t begin,end,writable;};
struct a7_menu_maps {uint32_t count,lines;struct a7_menu_range ranges[A7_MENU_MAP_RANGES_MAX];};
/* Reads no target memory and makes no syscalls. Every malformed/truncated map
 * input rejects the entire allowlist. Device mappings are never admitted. */
int a7_menu_parse_maps(const char *,uint32_t,struct a7_menu_maps *);
int a7_menu_maps_contains(const struct a7_menu_maps *,uint32_t,uint32_t);
#endif

int a7_menu_maps_writable(const struct a7_menu_maps *,uint32_t,uint32_t);
