#ifndef _EGUI_DIAG_H_
#define _EGUI_DIAG_H_

/**
 * @file egui_diag.h
 * @brief Self-contained diagnostic probes for platform compatibility paths.
 *
 * Designed to be invoked once at boot (or on demand) on any target board
 * (TC32 / Cortex-M / PC simulator) and print expected vs actual values via
 * egui_api_log. Useful when behavior differs on a specific compiler/platform:
 * comparing the printed table between PC and the failing board pinpoints the
 *
 * broken assumption (bitfield order, signed shift, struct size, alpha math,
 * packed data traversal, ...).
 * No GUI/canvas/PFB is required. All probes
 * operate on local stack data.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Run every probe in sequence. Prints a summary line at the end:
 *   "[DIAG] ==== end PASS fail=0 ====".
 *  Returns 0 if all probes match the expected golden values, otherwise the
 *  number of failures.
 */
int egui_diag_run_all(void);

/* Individual probes. Each prints expected vs actual lines and returns the
 * number of mismatches (0 == OK). They may be called separately to focus on
 * one suspected risk area on the customer's board.
 */
int egui_diag_probe_platform(void);          /* sizes, signedness, endianness */
int egui_diag_probe_bitfield(void);          /* egui_color_t bitfield layout  */
int egui_diag_probe_struct_layout(void);     /* font descriptor struct sizes  */
int egui_diag_probe_alpha_tables(void);      /* alpha lookup tables           */
int egui_diag_probe_alpha_mix(void);         /* egui_color_alpha_mix          */
int egui_diag_probe_rgb565_blend(void);      /* rgb_mix / rgb565 safe vs fast */
int egui_diag_probe_glyph_4bpp(void);        /* 4bpp packed glyph blend loop  */
int egui_diag_probe_format_flags(void);      /* FONT_FORMAT_* config flags    */
int egui_diag_probe_descriptor_lookup(void); /* code/char array lookup        */
int egui_diag_probe_glyph_1bpp(void);        /* 1bpp glyph row blend          */
int egui_diag_probe_bitmap_ptr(void);        /* base + uint32 idx pointer     */
int egui_diag_probe_alpha_table_full(void);  /* full table_4 == i*17 sweep    */
int egui_diag_probe_real_glyph_byte(void);   /* real H-row 0xe8 nibble split  */
int egui_diag_probe_real_glyph_row(void);    /* end-to-end H-row inner loop   */
int egui_diag_probe_rgb_mix_ptr(void);       /* egui_rgb_mix_ptr (hot path)   */
int egui_diag_probe_canvas_pfb_write(void);  /* simulated PFB pixel write     */

/* ---- Round 3: compiler-level / TC32-suspect probes ---- */
int egui_diag_probe_int_promotion(void);         /* uint8*uint8 widening to int   */
int egui_diag_probe_signed_unsigned_cmp(void);   /* (-1) < 1u semantics         */
int egui_diag_probe_int16_widen_mul(void);       /* int16*int16 -> int32          */
int egui_diag_probe_div_signed(void);            /* signed div / mod truncation   */
int egui_diag_probe_uint_wrap(void);             /* uint8/16 wraparound           */
int egui_diag_probe_shift_unsigned_neg(void);    /* (uint32_t)-1 >> 1            */
int egui_diag_probe_shift_high_count(void);      /* 1u << 31, byte-count shifts   */
int egui_diag_probe_misaligned_u16(void);        /* u16 read at odd offset        */
int egui_diag_probe_misaligned_u32(void);        /* u32 read at odd offset        */
int egui_diag_probe_byte_assemble_le(void);      /* manual 4-byte LE assemble     */
int egui_diag_probe_memcpy_4(void);              /* memcpy(4) vs *(u32*) cast     */
int egui_diag_probe_memmove_overlap(void);       /* memmove forward/backward      */
int egui_diag_probe_func_ptr_call(void);         /* fn-ptr through struct field   */
int egui_diag_probe_switch_sparse(void);         /* switch on font_bit_mode       */
int egui_diag_probe_const_addr_stable(void);     /* const data pointer identity   */
int egui_diag_probe_strcmp_basic(void);          /* manual byte-wise strcmp       */
int egui_diag_probe_alpha_mix_sweep(void);       /* alpha_mix exhaustive table    */
int egui_diag_probe_rgb_mix_alpha_sweep(void);   /* rgb_mix monotonic in alpha  */
int egui_diag_probe_glyph_4bpp_grid(void);       /* 16-nibble row, all alphas     */
int egui_diag_probe_glyph_4bpp_zero(void);       /* zero byte never writes        */
int egui_diag_probe_glyph_overdraw(void);        /* same pixel drawn twice        */
int egui_diag_probe_canvas_alpha_compose(void);  /* canvas_alpha * glyph_alpha */
int egui_diag_probe_negative_off_y(void);        /* int8 off_y sign-extend math   */
int egui_diag_probe_advance_sum(void);           /* sum of advances "Hello World!"*/
int egui_diag_probe_array_iter_19(void);         /* loop over 19 char descriptors */
int egui_diag_probe_pixel_idx_progression(void); /* idx[i+1]-idx[i] == size[i]*/
int egui_diag_probe_struct_field_via_cast(void); /* read field via raw bytes  */
int egui_diag_probe_local_static_zero(void);     /* static var default-zero       */
int egui_diag_probe_zero_pfb_init(void);         /* loop init = 0                 */
int egui_diag_probe_alpha_zero_skip(void);       /* canvas draw_point alpha=0     */
int egui_diag_probe_color_struct_size(void);     /* sizeof(egui_color_t) == 2     */
int egui_diag_probe_dim_signed(void);            /* egui_dim_t signedness         */
int egui_diag_probe_alpha_table_inverse(void);   /* table_4[i]/17 == i sweep    */
int egui_diag_probe_alpha_table_2_full(void);    /* table_2[i] == i*85 sweep     */
int egui_diag_probe_compile_config(void);        /* dump EGUI_CONFIG_* values     */

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DIAG_H_ */
