/* Fixed closed whitelist of backend paths, entry symbols, and ABI build tags.
 * Verified via ARM32 ET_DYN ELF header, dladdr() module ownership, and ABI build. */
static const struct backend_identity identities[3]={
{"/usr/lib/liba7r_audio_borrow_v2.so","a7r_audio_borrow_v2",0x29120502u},
{"/usr/lib/liba7r_audio_fresh_v1.so","a7r_audio_fresh_v1",0x29120503u},
{"/usr/lib/liba7r_capture_fresh_v12.so","a7r_capture_fresh_v12",0x3110d10cu},
};
