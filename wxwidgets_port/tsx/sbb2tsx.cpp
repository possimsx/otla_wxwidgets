// Direct SBB -> TSX (TZX 1.21) generator for MSX.
// See sbb2tsx.h for the high-level layout this file produces.

#include "sbb2tsx.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "../../source/sbb.h"
#include "../../source/wav2.h"
#include "../../source/loaders.h"
#include "../../source/funciones.h"   // checksum()

// loaders.cpp globals not exposed in loaders.h
extern "C++" {
    extern ts_sbb_header head_sbb;
    extern unsigned char reubica;
    extern int           enable_int;
}
// wav2.cpp globals not exposed in wav2.h
extern unsigned char c_pseudopause[19];

// Need block_type constants for the SBB iteration loop (sbb.h defines them).
// Common ones used here: _test_, _Snapshot4ZX48_, _Snapshot4ZX128_, _Snapshot4CPC_


// ============================================================================
// Constants
// ============================================================================

// Z80 reference clock used by the TZX spec for all T-state durations.
static const double Z80_CLOCK = 3500000.0;

// MSX KCS encoding (matches what block2msx_bin_wav already emits as audio):
//   - ZERO bit = 1 cycle of "low" tone   -> 2 pulses of 10 samples each (at freq)
//   - ONE  bit = 2 cycles of "high" tone -> 4 pulses of 5  samples each
//   - PILOT pulse = ONE pulse (high tone)
// Sample counts come from c_zero_msx[0]=20 / c_one_msx structure in wav2.cpp.
static const int  SAMPLES_PER_ZERO_PULSE = 10;
static const int  SAMPLES_PER_ONE_PULSE  = 5;
static const int  ZERO_PULSES_PER_BIT    = 2;
static const int  ONE_PULSES_PER_BIT     = 4;
static const int  SAMPLES_PER_BIT_CYCLE  = 20;     // c_zero_msx[0] / c_one_msx[0]

// Pilot cycle counts — match block2msx_bin_wav (loaders.cpp + wav2.cpp).
// One "cycle" of c_one_msx = 2 cycles of the high tone = 4 pulses.
static const int  LONG_PILOT_CYCLES_NORMAL = 8000;
static const int  LONG_PILOT_CYCLES_FAST   = 1222;
static const int  SHORT_PILOT_CYCLES_NORMAL = 2000;
static const int  SHORT_PILOT_CYCLES_FAST   = 1100;

// #4B byte-config byte:
//   bits 7-6: # leading bits  (1)
//   bit  5:   leading bit value (0 = start bit low)
//   bits 4-3: # trailing bits (2)
//   bit  2:   trailing bit value (1 = stop bits high)
//   bit  1:   reserved
//   bit  0:   endianness (0 = LSb first, matches byte2msx_wav)
//  = (1<<6) | (0<<5) | (2<<3) | (1<<2) | 0 = 0x54
static const uint8_t BYTE_CFG_MSX = 0x54;

// MSX BIOS file-type ID byte — written 10 times at the start of the cassette
// file header. 0xD0 = binary file (BSAVE/BLOAD).
static const uint8_t MSX_BINARY_ID = 0xD0;

// Pause (ms) the TSX player applies AFTER the block.
//
// Between header and data #4B: ~227 ms in the original WAV (1 zero + 500 ones
// of c_one_msx at 20 samples/cycle); 500 ms is a safe round-up that MSX BIOS
// still accepts.
//
// Between data #4B and the veloz #15: 0 ms — the next block (#15) already
// starts with the bridging pseudopause(50) audio (we no longer skip it from
// the WAV), so adding TSX-level silence on top would just stretch the gap
// and cause the OTLA loader to spin in its pre-pilot polling loop longer
// than necessary.
static const uint16_t PAUSE_AFTER_HEADER_MS = 500;
static const uint16_t PAUSE_AFTER_DATA_MS   = 0;


// ============================================================================
// Little-endian writers
// ============================================================================

static inline int w_u8 (FILE* f, uint8_t  v) { return fputc(v, f) == EOF ? -1 : 0; }
static inline int w_u16(FILE* f, uint16_t v)
{
    if (fputc(v & 0xFF, f) == EOF) return -1;
    if (fputc((v >> 8) & 0xFF, f) == EOF) return -1;
    return 0;
}
static inline int w_u24(FILE* f, uint32_t v)
{
    if (fputc(v & 0xFF, f) == EOF) return -1;
    if (fputc((v >> 8) & 0xFF, f) == EOF) return -1;
    if (fputc((v >> 16) & 0xFF, f) == EOF) return -1;
    return 0;
}
static inline int w_u32(FILE* f, uint32_t v)
{
    if (w_u16(f, v & 0xFFFF) != 0) return -1;
    return w_u16(f, v >> 16);
}
static inline int w_bytes(FILE* f, const void* p, size_t n)
{
    return fwrite(p, 1, n, f) == n ? 0 : -1;
}


// ============================================================================
// Block writers
// ============================================================================

// TSX/TZX 1.21 header: "ZXTape!\x1A", then major=1, minor=21.
static int write_tsx_header(FILE* f)
{
    if (w_bytes(f, "ZXTape!\x1A", 8) != 0) return -1;
    if (w_u8(f, 1) != 0) return -1;
    if (w_u8(f, 21) != 0) return -1;
    return 0;
}

// #30 — Text description block.
static int write_block_30(FILE* f, const char* text)
{
    if (!text) return 0;
    size_t len = strlen(text);
    if (len > 255) len = 255;
    if (w_u8(f, 0x30) != 0) return -1;
    if (w_u8(f, (uint8_t)len) != 0) return -1;
    return w_bytes(f, text, len);
}

// #4B — Kansas City Standard block.
//   layout after the 0x4B ID byte:
//     DWORD  block_length (= 12 + size, excluding the 4 length bytes themselves)
//     WORD   pause after block (ms)
//     WORD   pilot pulse duration in T-states
//     WORD   number of pilot pulses
//     WORD   zero-pulse duration in T-states
//     WORD   one-pulse duration in T-states
//     BYTE   bit config: (zero pulses per ZERO bit) << 4 | (one pulses per ONE bit)
//     BYTE   byte config: leading/trailing bits + endianness
//     BYTE[size] data
static int write_block_4B(FILE* f,
                          const uint8_t* data, size_t size,
                          uint16_t pause_ms,
                          uint16_t t_pilot_pulse,
                          uint16_t pilot_pulses,
                          uint16_t t_zero_pulse,
                          uint16_t t_one_pulse,
                          uint8_t  bit_cfg,
                          uint8_t  byte_cfg)
{
    if (w_u8(f,  0x4B) != 0) return -1;
    if (w_u32(f, (uint32_t)(12 + size)) != 0) return -1;
    if (w_u16(f, pause_ms) != 0) return -1;
    if (w_u16(f, t_pilot_pulse) != 0) return -1;
    if (w_u16(f, pilot_pulses) != 0) return -1;
    if (w_u16(f, t_zero_pulse) != 0) return -1;
    if (w_u16(f, t_one_pulse) != 0) return -1;
    if (w_u8(f,  bit_cfg) != 0) return -1;
    if (w_u8(f,  byte_cfg) != 0) return -1;
    return w_bytes(f, data, size);
}

// #15 — Direct Recording block.
//   layout after the 0x15 ID byte:
//     WORD     T-states per sample
//     WORD     pause after block (ms)
//     BYTE     used bits in last byte (1-8)
//     3 BYTES  block length (24-bit LE) = number of data bytes
//     BYTE[N]  packed sample bits, MSb first within each byte
static int write_block_15(FILE* f,
                          const uint8_t* packed_bits, uint32_t num_bits,
                          uint16_t t_per_sample,
                          uint16_t pause_ms)
{
    uint32_t num_bytes = (num_bits + 7) / 8;
    uint8_t  used_bits = num_bits % 8;
    if (used_bits == 0) used_bits = 8;

    if (w_u8(f, 0x15) != 0) return -1;
    if (w_u16(f, t_per_sample) != 0) return -1;
    if (w_u16(f, pause_ms) != 0) return -1;
    if (w_u8(f, used_bits) != 0) return -1;
    if (w_u24(f, num_bytes) != 0) return -1;
    return w_bytes(f, packed_bits, num_bytes);
}


// ============================================================================
// Bootstrap sample-count predictor
// ============================================================================
//
// Mirrors block2msx_bin_wav() in source/wav2.cpp. Returns the number of audio
// samples that constitute the MSX BIOS-loaded bootstrap (long pilot + file
// header + inter-block pause + short pilot + data + end marker). The
// pseudopause(50) that wavloader4msx() emits afterward is INTENTIONALLY NOT
// counted here — we want those bridging samples to land in the #15 block so
// the veloz signal flows continuously, matching what the original full-WAV
// path produced.
//
// Layout:
//   long pilot         : pilot_cycles_long  × 20 samples
//   file header        : 16 bytes × 11 bits/byte × 20 samples/bit
//   pause              : (1 zero + 500 ones) × 20 samples = 501 × 20
//   short pilot        : pilot_cycles_short × 20 samples
//   data               : data_bytes × 11 × 20
//   end marker         : 10 × 20
static long compute_bootstrap_samples(int loader_total_bytes)
{
    int long_cyc  = acelerar ? LONG_PILOT_CYCLES_FAST  : LONG_PILOT_CYCLES_NORMAL;
    int short_cyc = acelerar ? SHORT_PILOT_CYCLES_FAST : SHORT_PILOT_CYCLES_NORMAL;

    long s = 0;
    s += (long)long_cyc            * SAMPLES_PER_BIT_CYCLE;
    s += (long)16 * 11             * SAMPLES_PER_BIT_CYCLE;
    s += (long)501                 * SAMPLES_PER_BIT_CYCLE;
    s += (long)short_cyc           * SAMPLES_PER_BIT_CYCLE;
    s += (long)loader_total_bytes * 11 * SAMPLES_PER_BIT_CYCLE;
    s += (long)10                  * SAMPLES_PER_BIT_CYCLE;

    return s;
}


// ============================================================================
// Veloz payload as #15: synthesize via sbb2wav to a temp WAV, skip the
// bootstrap samples, pack the rest as 1-bit-per-sample data.
// ============================================================================

static int write_veloz_block_15(FILE* out_tsx,
                                const char* sbb_file,
                                int loader_total_bytes)
{
    // Build temp WAV path next to the output TSX.
    char tmp_wav[1024];
    snprintf(tmp_wav, sizeof(tmp_wav), "%s.tmp.wav", sbb_file);

    int rc = sbb2wav(sbb_file, tmp_wav);
    if (rc != 0) return rc;

    FILE* w = fopen(tmp_wav, "rb");
    if (!w) return -10;

    // Skip 44-byte WAV header
    if (fseek(w, 44, SEEK_SET) != 0) { fclose(w); remove(tmp_wav); return -11; }

    // Skip bootstrap samples (each sample = 1 byte, unsigned 8-bit PCM)
    long boot_samples = compute_bootstrap_samples(loader_total_bytes);
    if (fseek(w, boot_samples, SEEK_CUR) != 0) {
        fclose(w); remove(tmp_wav); return -12;
    }

    // Read remaining samples, quantize to 1 bit each (>= 0x80 -> 1, else 0),
    // pack MSb-first into bytes.
    // We allocate a buffer sized to the rest of the file.
    long here = ftell(w);
    fseek(w, 0, SEEK_END);
    long end = ftell(w);
    fseek(w, here, SEEK_SET);
    long veloz_samples = end - here;
    if (veloz_samples <= 0) {
        fclose(w); remove(tmp_wav); return -13;
    }

    long num_bytes = (veloz_samples + 7) / 8;
    uint8_t* packed = (uint8_t*)calloc((size_t)num_bytes, 1);
    if (!packed) { fclose(w); remove(tmp_wav); return -14; }

    for (long i = 0; i < veloz_samples; i++) {
        int c = fgetc(w);
        if (c == EOF) break;
        // Match the original wav2tzx polarity convention exactly:
        // the historical code used `if (sample > 0)` against a signed char,
        // i.e. bit = 1 for unsigned bytes 0x01..0x7F (low-audio half), bit = 0
        // for 0x80..0xFF (high-audio half). The TZX spec says bit = 1 means
        // EAR high, so this is technically "inverted" — but the OTLA loader's
        // SYNC step auto-detects polarity, and this is the polarity the loader
        // has been tuned against for years.
        if ((int8_t)(uint8_t)c > 0) {
            packed[i >> 3] |= (uint8_t)(0x80u >> (i & 7));
        }
    }
    fclose(w);
    remove(tmp_wav);

    // T-states per sample at the playback rate used by sbb2wav
    uint16_t t_per_sample = (uint16_t)(Z80_CLOCK / freq_muestreo + 0.5);

    rc = write_block_15(out_tsx, packed, (uint32_t)veloz_samples,
                        t_per_sample, PAUSE_AFTER_DATA_MS);
    free(packed);
    return rc;
}


// ============================================================================
// Veloz payload as #19 (Generalized Data Block)
// ============================================================================
//
// Replaces the original audio synthesis (block2veloz_wav) with a structured
// TZX 1.20 #19 block per veloz frame. Each #19 carries:
//   - pilot/sync section: 25 × pilot + 1 × sync + 1 × sync2
//   - data section: 3-bit symbol indices into a 7-symbol alphabet
//                   (c_00, c_01, c_10, c_11, c_t1, c_t2, c_t3)
//
// Why bother instead of just using #15 Direct Recording? Because #15 quantizes
// every transition to a sample boundary (~22.6 µs at 44.1 kHz/3.5 MHz), and at
// fast veloz speeds (e.g. 2.5 s/b → half-period ≈ 28 µs) that's nearly half a
// bit period of edge jitter — enough to corrupt the loader's symbol detection.
// #19 emits each pulse with an explicit T-state duration, so timing is exact
// regardless of how the original WAV would have been sampled.

// ---- Symbol indices in the data alphabet (ASD=7) ----
enum {
    SYM_C00 = 0, SYM_C01 = 1, SYM_C10 = 2, SYM_C11 = 3,
    SYM_T1  = 4, SYM_T2  = 5, SYM_T3  = 6,
    DATA_ALPHABET_SIZE = 7,
    DATA_BITS_PER_SYMBOL = 3,
};

// ---- Symbol indices in the pilot/sync alphabet (ASP=3) ----
enum {
    PSYM_PILOT = 0, PSYM_SYNC = 1, PSYM_SYNC2 = 2,
    PILOT_ALPHABET_SIZE = 3,
};

// Cycle widths in samples, replicating make_wave_tables() in wav2.cpp so we
// don't need to call open_wav() just to populate them.
struct VelozWidths {
    int c00, c01, c10, c11, t1, t2, t3;
    int pilot_cycle;  // c_pilot_velozwav default = 18
    int sync_cycle;   // c_sync_velozwav  default = 8
    int sync2_cycle;  // = c_10
};
static void compute_veloz_widths(int spb, VelozWidths* w)
{
    int inc = (spb & 1) ? 1 : 2;
    int c00 = (spb + 1) / 2 - 1 - inc;
    if (spb == 15 || spb == 17) { c00 -= 3; inc = 3; }
    w->c00 = c00;
    w->c01 = c00 + inc;
    w->c10 = c00 + 2 * inc;
    w->c11 = c00 + 3 * inc;
    w->t1  = w->c11 + 2 * (inc == 3);
    w->t2  = w->t1 + 2;
    w->t3  = w->t2 + 1;
    w->pilot_cycle = 18;
    w->sync_cycle  = 8;
    w->sync2_cycle = w->c10;
}

// Number of T-states for a half-cycle of `samples` samples at freq_muestreo.
static inline uint16_t halfcycle_to_T(int samples)
{
    return (uint16_t)((double)samples * Z80_CLOCK / freq_muestreo + 0.5);
}

// Append a SYMDEF (flag + 2 pulses) into a growable buffer.
// All veloz symbols use behavior=00 (alternating polarity, default).
static void append_symdef(std::vector<uint8_t>& buf, int cycle_samples)
{
    int t0 = cycle_samples / 2;
    int t1 = cycle_samples - t0;
    uint16_t p0 = halfcycle_to_T(t0);
    uint16_t p1 = halfcycle_to_T(t1);
    buf.push_back(0x00);                  // flag
    buf.push_back(p0 & 0xFF); buf.push_back(p0 >> 8);
    buf.push_back(p1 & 0xFF); buf.push_back(p1 >> 8);
}

// Append a PRLE entry (symbol index + 2-byte repetition count) into a buffer.
static void append_prle(std::vector<uint8_t>& buf, uint8_t sym, uint16_t reps)
{
    buf.push_back(sym);
    buf.push_back(reps & 0xFF);
    buf.push_back(reps >> 8);
}

// MSb-first bit packer for the #19 data stream. Holds an accumulator until
// 8 bits are present, then emits a byte.
struct BitPacker {
    std::vector<uint8_t>& out;
    uint32_t accum;
    int bits;
    BitPacker(std::vector<uint8_t>& o) : out(o), accum(0), bits(0) {}
    void push(int value, int width)   // value in [0, 2^width)
    {
        accum = (accum << width) | (uint32_t)(value & ((1 << width) - 1));
        bits += width;
        while (bits >= 8) {
            bits -= 8;
            out.push_back((uint8_t)((accum >> bits) & 0xFF));
        }
    }
    void flush()
    {
        if (bits > 0) {
            uint8_t b = (uint8_t)((accum << (8 - bits)) & 0xFF);
            out.push_back(b);
            bits = 0;
            accum = 0;
        }
    }
};

// Emit one veloz frame as a TZX #19 block.
//
//   payload = checksum + N data bytes + 4 tail markers (c_t1, c_t2, c_t3, c_t3)
//   pilot/sync = 25 pilot + 1 sync + 1 sync2
//
// pause_ms goes into the #19's "pause after block" field.
static int write_veloz_frame_19(FILE* out_tsx,
                                const VelozWidths& w,
                                const unsigned char* data, int length,
                                uint16_t pause_ms)
{
    // ---- Pilot/sync section: PRLE stream of pilot×25, sync×1, sync2×1 ----
    std::vector<uint8_t> pilot_symdefs;
    append_symdef(pilot_symdefs, w.pilot_cycle);
    append_symdef(pilot_symdefs, w.sync_cycle);
    append_symdef(pilot_symdefs, w.sync2_cycle);

    std::vector<uint8_t> pilot_prle;
    append_prle(pilot_prle, PSYM_PILOT,  25);
    append_prle(pilot_prle, PSYM_SYNC,   1);
    append_prle(pilot_prle, PSYM_SYNC2,  1);

    const uint32_t TOTP = 3;
    const uint8_t  NPP  = 2;
    const uint8_t  ASP  = PILOT_ALPHABET_SIZE;

    // ---- Data section: SYMDEFs for c_00..c_11, c_t1..c_t3, then bit stream ----
    std::vector<uint8_t> data_symdefs;
    append_symdef(data_symdefs, w.c00);
    append_symdef(data_symdefs, w.c01);
    append_symdef(data_symdefs, w.c10);
    append_symdef(data_symdefs, w.c11);
    append_symdef(data_symdefs, w.t1);
    append_symdef(data_symdefs, w.t2);
    append_symdef(data_symdefs, w.t3);

    std::vector<uint8_t> data_stream;
    BitPacker bp(data_stream);

    unsigned char cks = checksum(data, length);

    // Loader expects: checksum byte (4 syms), then N data bytes (4 syms each),
    // then 4 tail symbols (t1, t2, t3, t3).
    uint32_t TOTD = 4u * (1u + (uint32_t)length) + 4u;

    auto push_byte_as_4_syms = [&](unsigned char b) {
        bp.push((b >> 6) & 3, DATA_BITS_PER_SYMBOL);
        bp.push((b >> 4) & 3, DATA_BITS_PER_SYMBOL);
        bp.push((b >> 2) & 3, DATA_BITS_PER_SYMBOL);
        bp.push( b       & 3, DATA_BITS_PER_SYMBOL);
    };

    push_byte_as_4_syms(cks);
    for (int i = 0; i < length; i++)
        push_byte_as_4_syms((unsigned char)data[i]);

    // Tail
    bp.push(SYM_T1, DATA_BITS_PER_SYMBOL);
    bp.push(SYM_T2, DATA_BITS_PER_SYMBOL);
    bp.push(SYM_T3, DATA_BITS_PER_SYMBOL);
    bp.push(SYM_T3, DATA_BITS_PER_SYMBOL);
    bp.flush();

    const uint8_t NPD = 2;
    const uint8_t ASD = DATA_ALPHABET_SIZE;

    // ---- Assemble the block ----
    //   2 pause + 4 TOTP + 1 NPP + 1 ASP + 4 TOTD + 1 NPD + 1 ASD
    //   + pilot_symdefs + pilot_prle + data_symdefs + data_stream
    uint32_t variable_size = (uint32_t)(pilot_symdefs.size() + pilot_prle.size()
                                       + data_symdefs.size()  + data_stream.size());
    uint32_t block_size    = 14 + variable_size;   // 14 = fixed fields above

    if (w_u8(out_tsx,  0x19) != 0) return -1;
    if (w_u32(out_tsx, block_size) != 0) return -1;
    if (w_u16(out_tsx, pause_ms) != 0) return -1;
    if (w_u32(out_tsx, TOTP) != 0) return -1;
    if (w_u8(out_tsx,  NPP)  != 0) return -1;
    if (w_u8(out_tsx,  ASP)  != 0) return -1;
    if (w_u32(out_tsx, TOTD) != 0) return -1;
    if (w_u8(out_tsx,  NPD)  != 0) return -1;
    if (w_u8(out_tsx,  ASD)  != 0) return -1;

    if (w_bytes(out_tsx, pilot_symdefs.data(), pilot_symdefs.size()) != 0) return -1;
    if (w_bytes(out_tsx, pilot_prle.data(),    pilot_prle.size())    != 0) return -1;
    if (w_bytes(out_tsx, data_symdefs.data(),  data_symdefs.size())  != 0) return -1;
    if (w_bytes(out_tsx, data_stream.data(),   data_stream.size())   != 0) return -1;
    return 0;
}

// Iterate SBB blocks, emit two #19 frames per block (veloz_header + data).
// Matches sbb2wav's main loop for the MSX path.
static int write_veloz_block_19(FILE* out_tsx,
                                const char* sbb_file,
                                int /*loader_total_bytes*/)
{
    VelozWidths w;
    compute_veloz_widths(s_p_b, &w);

    FILE* sf = fopen(sbb_file, "rb");
    if (!sf) return -20;

    // Skip the SBB header (we already loaded head_sbb earlier in sbb2tsx)
    if (fseek(sf, sizeof(ts_sbb_header), SEEK_SET) != 0) { fclose(sf); return -21; }

    // First, count total blocks so we can pick the right "last-block" opcode
    // for veloz_header[2] just like sbb2wav does.
    int block_count = 0;
    {
        ts_sbb_block tmp;
        long pos = ftell(sf);
        while (fread(&tmp, 1, SIZE_BLOCK_HEADER, sf) == SIZE_BLOCK_HEADER) {
            if (fseek(sf, tmp.size, SEEK_CUR) != 0) break;
            block_count++;
        }
        fseek(sf, pos, SEEK_SET);
    }

    unsigned short direccion = reubica * 0x100;
    // ts_sbb_block has an inline 64 KB data buffer — too big for stack.
    ts_sbb_block* blk = (ts_sbb_block*)malloc(sizeof(ts_sbb_block));
    if (!blk) { fclose(sf); return -22; }

    int b = 1;
    int rc = 0;
    while (rc == 0 && fread(blk, 1, SIZE_BLOCK_HEADER, sf) == SIZE_BLOCK_HEADER)
    {
        if (fread(blk->data, 1, blk->size, sf) != (unsigned)blk->size) { rc = -23; break; }

        // Address-truncation safety (matches sbb2wav)
        if ((blk->ini < direccion) && (direccion < blk->ini + blk->size))
            blk->size = direccion - blk->ini;

        // Build the 5-byte veloz_header as sbb2wav does
        unsigned char veloz_header[5];
        veloz_header[0] = (unsigned char)((blk->ini - 1) & 0xff);
        veloz_header[1] = (unsigned char)((blk->ini - 1) >> 8);
        veloz_header[3] = (unsigned char)(blk->exec & 0xff);
        veloz_header[4] = (unsigned char)(blk->exec >> 8);

        bool last_block = (b == head_sbb.n_blocks);
        if (last_block) {
            if (head_sbb.machine[0] == 'Z') {
                if (head_sbb.usr_pc) {
                    veloz_header[2] = 0xc3;
                    veloz_header[3] = (unsigned char)(head_sbb.usr_pc & 0xff);
                    veloz_header[4] = (unsigned char)(head_sbb.usr_pc >> 8);
                } else {
                    veloz_header[2] = 0xc9;
                }
            } else if (head_sbb.machine[0] == 'M') {
                // MSX: always RET so the loader's start_cassette exit path
                // runs motor/caps-off + jp (HL) where HL = (bu+1) = exec.
                // Uniform for exec=0 and exec!=0; lets motor-off run.
                veloz_header[2] = 0xc9;
            } else {
                veloz_header[2] = blk->exec ? 0xc3 : 0xc9;
            }
        } else {
            veloz_header[2] = blk->exec ? 0xcd : 0x01;
        }

        // header frame -> next frame is the data, no pause needed
        rc = write_veloz_frame_19(out_tsx, w, veloz_header, 5, 0);
        if (rc != 0) break;

        // data frame -> pause = pausa_ms before the next block (or 0 if last)
        uint16_t pause = last_block ? 0 : (uint16_t)pausa_ms;
        rc = write_veloz_frame_19(out_tsx, w, blk->data, blk->size, pause);
        b++;
    }

    free(blk);
    fclose(sf);
    return rc;
}



// ============================================================================
// Public entry point
// ============================================================================

// Above 44.1 kHz, openMSX's TsxParser::writeSample accumulator produces enough
// pulse-width jitter to break veloz symbol detection in the #15 path. Clamping
// the encoding rate while sbb2tsx runs keeps every downstream consumer
// (wavloader4msx_build, sbb2wav, the #4B T-state math, halfcycle_to_T) on the
// same value, and the dtor restores the user's setting on every exit path.
namespace {
struct FreqClamp {
    int saved;
    explicit FreqClamp(int cap) : saved(freq_muestreo) {
        if (freq_muestreo > cap) freq_muestreo = cap;
    }
    ~FreqClamp() { freq_muestreo = saved; }
};
}

int sbb2tsx(const char* sbb_file,
            const char* tsx_file,
            const sbb2tsx_options* options)
{
    if (!sbb_file || !tsx_file) return -1;

    // Load the SBB header so globals (head_sbb, reubica, enable_int) are set
    // exactly as wavloader4msx_build expects.
    FILE* sf = fopen(sbb_file, "rb");
    if (!sf) return -2;
    if (fread(&head_sbb, 1, sizeof(head_sbb), sf) != sizeof(head_sbb)) {
        fclose(sf);
        return -3;
    }
    fclose(sf);

    if (head_sbb.machine[0] != 'M') return -4;  // MSX only

    reubica    = head_sbb.locate;
    enable_int = head_sbb.ei_di;

    FreqClamp freq_clamp(44100);

    // Pre-build the loader image so we have the exact bytes to put in the
    // two #4B blocks (file header + data). Same call mirrors what sbb2wav
    // does internally before emitting the bootstrap audio.
    int loader_total = 0;
    int rc = wavloader4msx_build(&loader_total);
    if (rc != 0) return rc;

    // Open the output file
    FILE* out = fopen(tsx_file, "wb");
    if (!out) return -5;

    if (write_tsx_header(out) != 0) goto fail;

    if (options && options->description) {
        if (write_block_30(out, options->description) != 0) goto fail;
    }

    {
        // Pilot/zero/one pulse durations in T-states, derived from the
        // sample lengths used by c_zero_msx / c_one_msx at freq_muestreo.
        uint16_t t_zero  = (uint16_t)(SAMPLES_PER_ZERO_PULSE / (double)freq_muestreo * Z80_CLOCK + 0.5);
        uint16_t t_one   = (uint16_t)(SAMPLES_PER_ONE_PULSE  / (double)freq_muestreo * Z80_CLOCK + 0.5);
        uint8_t  bit_cfg = (uint8_t)((ZERO_PULSES_PER_BIT << 4) | ONE_PULSES_PER_BIT);

        int long_cyc  = acelerar ? LONG_PILOT_CYCLES_FAST  : LONG_PILOT_CYCLES_NORMAL;
        int short_cyc = acelerar ? SHORT_PILOT_CYCLES_FAST : SHORT_PILOT_CYCLES_NORMAL;

        // Each c_one_msx cycle (written by long/short pilot) is 4 pulses
        // (2 cycles of the high tone, 2 pulses per cycle).
        uint16_t pilot_pulses_long  = (uint16_t)(long_cyc  * ONE_PULSES_PER_BIT);
        uint16_t pilot_pulses_short = (uint16_t)(short_cyc * ONE_PULSES_PER_BIT);

        // #4B Block A: MSX cassette file header — 10 file-type bytes + 6 name bytes.
        uint8_t file_hdr[16];
        memset(file_hdr, MSX_BINARY_ID, 10);
        memcpy(file_hdr + 10, head_sbb.nombre, 6);
        if (write_block_4B(out,
                           file_hdr, sizeof(file_hdr),
                           PAUSE_AFTER_HEADER_MS,
                           t_one, pilot_pulses_long,
                           t_zero, t_one,
                           bit_cfg, BYTE_CFG_MSX) != 0) goto fail;

        // #4B Block B: BSAVE 6-byte header + loader code, in loader[0..total-1].
        if (write_block_4B(out,
                           loader, (size_t)loader_total,
                           PAUSE_AFTER_DATA_MS,
                           t_one, pilot_pulses_short,
                           t_zero, t_one,
                           bit_cfg, BYTE_CFG_MSX) != 0) goto fail;
    }

    // Veloz payload
    if (options && options->mode == SBB2TSX_MODE_4B_PLUS_19) {
        if (write_veloz_block_19(out, sbb_file, loader_total) != 0) goto fail;
    } else {
        if (write_veloz_block_15(out, sbb_file, loader_total) != 0) goto fail;
    }

    fclose(out);
    return 0;

fail:
    fclose(out);
    remove(tsx_file);
    return -100;
}
