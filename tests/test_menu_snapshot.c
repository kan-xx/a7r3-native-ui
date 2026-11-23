#include "menu_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_BASE 0x10000u
#define MEM_SIZE 0x8000u

static uint8_t mem[MEM_SIZE];
static unsigned checks;

static void ok(int cond, const char *msg) {
    checks++;
    if (!cond) {
        fprintf(stderr, "FAIL [%u]: %s\n", checks, msg);
        exit(1);
    }
}

static void put32(uint32_t addr, uint32_t val) {
    memcpy(mem + (addr - MEM_BASE), &val, 4);
}

static void put_str(uint32_t addr, const char *s) {
    memcpy(mem + (addr - MEM_BASE), s, strlen(s) + 1);
}

static int mock_read(void *ctx, uint32_t addr, void *dst, uint32_t n) {
    (void)ctx;
    if (addr < MEM_BASE || n > MEM_SIZE || addr - MEM_BASE > MEM_SIZE - n) {
        return 0;
    }
    memcpy(dst, mem + (addr - MEM_BASE), n);
    return 1;
}

static void build_valid_scene(void) {
    memset(mem, 0, sizeof(mem));
    uint32_t p_vm_global = 0x10100u;
    uint32_t vm_addr = 0x10200u;
    uint32_t reg_singleton = 0x10400u;
    uint32_t bank0_addr = 0x11000u;
    uint32_t bank1_addr = 0x11400u;
    uint32_t view_aa = 0x12000u;
    uint32_t view_ffc = 0x12100u;
    uint32_t node_aa = 0x13000u;
    uint32_t node_ffc = 0x13100u;
    uint32_t str_mod_aa = 0x14000u;
    uint32_t str_fac_aa = 0x14100u;
    uint32_t str_mod_ffc = 0x14200u;
    uint32_t str_fac_ffc = 0x14300u;

    put32(p_vm_global, vm_addr);
    /* ViewManager header (28 words) */
    put32(vm_addr + 0 * 4, 39u);             /* configured_slots = 39 */
    put32(vm_addr + 2 * 4, reg_singleton);   /* registry pointer */
    put32(vm_addr + 7 * 4, 0x15000u);        /* app_config */
    put32(vm_addr + 8 * 4, bank0_addr);      /* bank 0 slots */
    put32(vm_addr + 9 * 4, bank1_addr);      /* bank 1 slots */

    /* Active slot 0 in bank 0: ViewMenu 0xAA; slot 1 in bank 0: View 0xFFC */
    put32(bank0_addr + 0 * 16 + 0, 1u);
    put32(bank0_addr + 0 * 16 + 4, view_aa);
    put32(bank0_addr + 0 * 16 + 8, 0xaau);
    put32(bank0_addr + 0 * 16 + 12, 0x10u);
    put32(view_aa, 0x16863890u);

    put32(bank0_addr + 1 * 16 + 0, 2u);
    put32(bank0_addr + 1 * 16 + 4, view_ffc);
    put32(bank0_addr + 1 * 16 + 8, 0xffcu);
    put32(bank0_addr + 1 * 16 + 12, 0x20u);
    put32(view_ffc, 0x20001000u);

    /* Registry doubly-linked list: sentinel (reg_singleton + 4) <-> node_aa <-> node_ffc */
    uint32_t sentinel = reg_singleton + 4u;
    put32(reg_singleton + 0, 0x05eef000u);   /* registry vptr */
    put32(reg_singleton + 4, node_aa);       /* head */
    put32(reg_singleton + 8, node_ffc);      /* tail */

    put_str(str_mod_aa, "viewUnified2.so");
    put_str(str_fac_aa, "ViewMenuToInstance");
    put_str(str_mod_ffc, "liba7r_menu_product_v21.so");
    put_str(str_fac_ffc, "A7ProductViewFactory21");

    /* node: [next, prev, reserved, view_id, module_str, factory_str] */
    put32(node_aa + 0, node_ffc);
    put32(node_aa + 4, sentinel);
    put32(node_aa + 12, 0xaau);
    put32(node_aa + 16, str_mod_aa);
    put32(node_aa + 20, str_fac_aa);

    put32(node_ffc + 0, sentinel);
    put32(node_ffc + 4, node_aa);
    put32(node_ffc + 12, 0xffcu);
    put32(node_ffc + 16, str_mod_ffc);
    put32(node_ffc + 20, str_fac_ffc);
}

int main(void) {
    struct a7_menu_snapshot snap;

    /* 1. Happy path: 39-slot ViewManager + View 0xFFC registry resolution */
    build_valid_scene();
    int rc = a7_menu_take_snapshot(mock_read, 0, 0x10100u, 0x10400u, 0xffcu, &snap);
    ok(rc == A7_MENU_OK, "valid snapshot returns OK");
    ok(snap.configured_slots == 39u && snap.registry_is_singleton == 1u, "39 slots & singleton verified");
    ok(snap.active_count == 2u && snap.registry_count == 2u, "two active views & two registry nodes");
    ok(snap.selected_count == 1u && !strcmp(snap.selected_factory, "A7ProductViewFactory21"), "0xFFC factory resolved");
    ok(!strcmp(snap.active[1].module, "liba7r_menu_product_v21.so"), "active slot 1 module string matched");

    /* 2. Duplicate ID detection */
    put32(0x13000u + 12, 0xffcu);
    rc = a7_menu_take_snapshot(mock_read, 0, 0x10100u, 0x10400u, 0xffcu, &snap);
    ok(rc == A7_MENU_DUPLICATE_ID, "duplicate view ID rejected");

    /* 3. Broken doubly-linked prev pointer detection */
    build_valid_scene();
    put32(0x13100u + 4, 0x10404u);
    rc = a7_menu_take_snapshot(mock_read, 0, 0x10100u, 0x10400u, 0xffcu, &snap);
    ok(rc == A7_MENU_BAD_LINK, "corrupted prev link rejected");

    /* 4. Unaligned argument rejection */
    rc = a7_menu_take_snapshot(mock_read, 0, 0x10102u, 0x10400u, 0xffcu, &snap);
    ok(rc == A7_MENU_BAD_ARG, "unaligned global pointer rejected");

    printf("MENU_SNAPSHOT passed %u assertions (39-slot ViewManager, PVM registry, duplicate & link guards)\n", checks);
    return 0;
}
