// Direct SBB -> TSX (TZX v1.21) generator for MSX.
//
// TSX is TZX 1.21 + the new #4B Kansas City block. This file produces a TSX
// containing:
//   1. TSX header (ZXTape! magic, version 1.21)
//   2. #30 text description (optional)
//   3. #4B "MSX file header" block (10 file-type bytes + 6 name bytes)
//   4. #4B "MSX data" block (BSAVE header + OTLA loader code)
//   5. Veloz payload, encoded either as:
//        - #15 Direct Recording (raw 1-bit samples)         when use_block19 = false
//        - #19 Generalized data block (4-symbol alphabet)   when use_block19 = true
//
// No external tools, no WAV intermediate.

#ifndef SBB2TSX_H
#define SBB2TSX_H

// Veloz-payload encoding choices for sbb2tsx().
enum sbb2tsx_mode
{
    SBB2TSX_MODE_4B_PLUS_15 = 0,   // #4B bootstrap + #15 payload  (more compatible - OpenMSX)
    SBB2TSX_MODE_4B_PLUS_19 = 1,   // #4B bootstrap + #19 payload  (more efficient - Maxduino)
};

struct sbb2tsx_options
{
    sbb2tsx_mode mode;
    const char*  description;   // nullptr or short string for the #30 text block
};

// Convert an SBB file to a TSX file. Returns 0 on success, non-zero on failure.
// On entry, the global wav/loaders state (freq_muestreo, s_p_b, make_loader,
// etc.) must already be set — typically by calling set_wav_vars() in
// MainFrame.cpp before this.
int sbb2tsx(const char* sbb_file,
            const char* tsx_file,
            const sbb2tsx_options* options);

#endif // SBB2TSX_H
