; MSX RAM Setup Routine
; Position-independent
.org 0

SSLTREG:    equ 0A8h
SUBSLTREG:  equ 0FFFFh
;MAPPER_P0:  equ 0FCh
MAPPER_P1:  equ 0FDh
MAPPER_P2:  equ 0FEh

msx_ram_setup:
        ; Step 1: Set pages 1-2 to page 3's slot
        in      a, (SSLTREG)
        ld      b, a            ; Save original
        rlca
        rlca                    ; Page 3 slot now in bits 0-1
        and     03h
        ld      c, a            ; C = page 3 slot
        
        ; Build: C | (C<<2) | (C<<4) | (C<<6) = slot in all pages
        add     a, a
        add     a, a
        or      c               ; Bits 0-3: slot in pages 0,1
        ld      c, a
        add     a, a
        add     a, a
        add     a, a
        add     a, a
        or      c               ; All 4 pages = same slot
        
        ; Restore page 0 from original
        and     0FCh            ; Clear page 0 bits
        ld      c, a
        ld      a, b
        and     03h             ; Original page 0
        or      c
        out     (SSLTREG), a

        ; Step 2: Set mapper segments
        ld      a, 2
        out     (MAPPER_P1), a
        dec     a               ; A = 1
        out     (MAPPER_P2), a

        ret
