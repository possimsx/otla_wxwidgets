
#ifndef loadersH
#define loadersH


#include "sbb.h"



extern   double duracion_loader;
extern   double duracion_total;
extern   char nombre_head[17];

extern  int make_loader;
extern  int checkloaderror;
extern  int motor;




extern int pausa_ms;


int sbb2wav (const char * filen_in,const char * filen_out);

// Builds the MSX loader image into the global `loader[]` array
// (BSAVE header at [0..5] + code at [6..]) using current head_sbb /
// freq_muestreo / s_p_b globals. Returns 0 on success and writes the
// total byte count (6 + longitud, possibly extended by mapper-init)
// into *out_total_bytes. Does NOT emit any WAV samples — for use by the
// direct SBB->TSX path that encodes the bootstrap as #4B blocks.
int wavloader4msx_build (int* out_total_bytes);

extern unsigned char loader[350];

/*
int azimut2wav (const char * file_out);
int calibZ81_wav (const char * file_out);
*/

#endif