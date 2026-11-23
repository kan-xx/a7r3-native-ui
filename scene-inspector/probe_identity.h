/* Mechanically generated from exact original ELF files; do not hand-edit. */
#ifndef PROBE_IDENTITY_H
#define PROBE_IDENTITY_H
#include <stdint.h>
static const struct {const char*path,*sha;unsigned bytes;} libs[3]={
{"/usr/lib/libObj.so","94ce6ec47c2b394aa1a4f781bfb3e9ec3337f6a2b8558fa025f42e87b2dda19b",19483804u},
{"/usr/lib/viewUnified2.so","9b243bb9c11acc1adfc09fb752e12835ea0a4556882867c53f1dd6c9143128d9",10752148u},
{"/usr/lib/viewUnified3.so","dcc03f8830f0f4f0e9f5869dd2cb97f81521ed903119d708ea044345b43dde99",689020u},
};
static const struct {unsigned module;uint32_t offset;uint8_t bytes[16];} anchors[]={
{0,0x5e2340,{176,181,0,175,4,70,255,247,235,255,56,177,32,29,247,247}},
{0,0x79c37e,{192,104,128,181,32,240,3,0,0,175,128,189,128,181,4,48}},
{0,0x3ecbc0,{128,181,0,175,208,248,76,1,128,189,128,181,0,175,144,248}},
{1,0x1f0ffc,{176,181,79,244,212,112,0,175,13,70,98,247,36,237,41,70}},
{2,0x40c44,{176,181,79,244,162,112,0,175,13,70,219,247,224,233,41,70}},
};
static const struct {unsigned id,module;uint32_t vptr;unsigned app;} classes[]={
{1,0,0x11ed7a0,1},
{2,0,0x11ec968,1},
{3,0,0x11ebe08,1},
{4,0,0x11eab90,1},
{5,0,0x11f0690,1},
{6,1,0x8b33a8,1},
{7,1,0x8c50d0,1},
{8,1,0x8b3ad0,1},
{9,1,0x8c5ba8,1},
{10,1,0x8c3008,1},
{11,1,0x8bf088,1},
{12,1,0x8bd6b0,1},
{13,1,0x8b41e0,1},
{14,1,0x8b4cb0,1},
{15,1,0x8bc9d0,1},
{16,1,0x8b7ad0,1},
{17,1,0x8bc450,1},
{18,1,0x8c5648,1},
{19,1,0x8b7560,1},
{20,1,0x8b4928,1},
{21,1,0x8bb7c0,1},
{22,1,0x8b8030,1},
{23,1,0x8bbee8,1},
{24,1,0x8c28d0,1},
{25,1,0x8b69e8,1},
{26,1,0x8c26f8,1},
{27,1,0x8b4e80,1},
{28,1,0x8bda48,1},
{29,0,0x11ea7c0,0},
};
#endif
