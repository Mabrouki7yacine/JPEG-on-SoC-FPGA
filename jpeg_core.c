#include "jpeg_core.h"

uint32_t RGB2YCbCr(const RGB* RGB_stream, const uint16_t block_size, uint8_t *Y_Channel, uint8_t *Cb_Channel, uint8_t *Cr_Channel)
{
    assert((block_size % 16) == 0);

    assert(RGB_stream != NULL);
    assert(Y_Channel  != NULL);
    assert(Cb_Channel != NULL);
    assert(Cr_Channel != NULL);

    for (uint32_t i = 0; i < block_size; i+=16)
    {
        uint8x16x3_t rgb = vld3q_u8((const uint8_t *)&RGB_stream[i]);
        uint8x16_t r = rgb.val[0];
        uint8x16_t g = rgb.val[1];
        uint8x16_t b = rgb.val[2];

        /* 16 × uint8 → two groups of 8 × uint8 */
        uint8x8_t r_low  = vget_low_u8(r);
        uint8x8_t r_high = vget_high_u8(r);

        uint8x8_t g_low  = vget_low_u8(g);
        uint8x8_t g_high = vget_high_u8(g);

        uint8x8_t b_low  = vget_low_u8(b);
        uint8x8_t b_high = vget_high_u8(b);

        /* 8 × uint8 → 8 × uint16 */
        uint16x8_t r16_l = vmovl_u8(r_low);
        uint16x8_t r16_h = vmovl_u8(r_high);

        uint16x8_t g16_l = vmovl_u8(g_low);
        uint16x8_t g16_h = vmovl_u8(g_high);

        uint16x8_t b16_l = vmovl_u8(b_low);
        uint16x8_t b16_h = vmovl_u8(b_high);

        /* Split each 8 × uint16 vector into two 4 × uint16 vectors */
        uint16x4_t r16_l_l = vget_low_u16(r16_l);
        uint16x4_t r16_l_h = vget_high_u16(r16_l);
        uint16x4_t r16_h_l = vget_low_u16(r16_h);
        uint16x4_t r16_h_h = vget_high_u16(r16_h);

        uint16x4_t g16_l_l = vget_low_u16(g16_l);
        uint16x4_t g16_l_h = vget_high_u16(g16_l);
        uint16x4_t g16_h_l = vget_low_u16(g16_h);
        uint16x4_t g16_h_h = vget_high_u16(g16_h);

        uint16x4_t b16_l_l = vget_low_u16(b16_l);
        uint16x4_t b16_l_h = vget_high_u16(b16_l);
        uint16x4_t b16_h_l = vget_low_u16(b16_h);
        uint16x4_t b16_h_h = vget_high_u16(b16_h);

        /* 4 × uint16 → 4 × uint32 */
        uint32x4_t r32_0 = vmovl_u16(r16_l_l);  /* pixels 0–3   */
        uint32x4_t r32_1 = vmovl_u16(r16_l_h);  /* pixels 4–7   */
        uint32x4_t r32_2 = vmovl_u16(r16_h_l);  /* pixels 8–11  */
        uint32x4_t r32_3 = vmovl_u16(r16_h_h);  /* pixels 12–15 */

        uint32x4_t g32_0 = vmovl_u16(g16_l_l);
        uint32x4_t g32_1 = vmovl_u16(g16_l_h);
        uint32x4_t g32_2 = vmovl_u16(g16_h_l);
        uint32x4_t g32_3 = vmovl_u16(g16_h_h);

        uint32x4_t b32_0 = vmovl_u16(b16_l_l);
        uint32x4_t b32_1 = vmovl_u16(b16_l_h);
        uint32x4_t b32_2 = vmovl_u16(b16_h_l);
        uint32x4_t b32_3 = vmovl_u16(b16_h_h);

        /* 4 × uint32 → 4 × float32 */ 
        float32x4_t r32_0f = vcvtq_f32_u32(r32_0);
        float32x4_t r32_1f = vcvtq_f32_u32(r32_1);
        float32x4_t r32_2f = vcvtq_f32_u32(r32_2);
        float32x4_t r32_3f = vcvtq_f32_u32(r32_3);

        float32x4_t g32_0f = vcvtq_f32_u32(g32_0);
        float32x4_t g32_1f = vcvtq_f32_u32(g32_1);
        float32x4_t g32_2f = vcvtq_f32_u32(g32_2);
        float32x4_t g32_3f = vcvtq_f32_u32(g32_3);

        float32x4_t b32_0f = vcvtq_f32_u32(b32_0);
        float32x4_t b32_1f = vcvtq_f32_u32(b32_1);
        float32x4_t b32_2f = vcvtq_f32_u32(b32_2);
        float32x4_t b32_3f = vcvtq_f32_u32(b32_3);
        /********* start with Luminance *********/ 
        {    
            float32x4_t vRedCoeff = vdupq_n_f32(Coeff_Y_R);
            float32x4_t vGrnCoeff = vdupq_n_f32(Coeff_Y_G);
            float32x4_t vBluCoeff = vdupq_n_f32(Coeff_Y_B);

            float32x4_t vOutput;
            vOutput = vmulq_f32(r32_0f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_0f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_0f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(0.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v0 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_1f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_1f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_1f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(0.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v1 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_2f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_2f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_2f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(0.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v2 = vcvtq_u32_f32(vOutput);
            
            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_3f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_3f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_3f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(0.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v3 = vcvtq_u32_f32(vOutput);

            uint16x4_t h0 = vqmovn_u32(v0);
            uint16x4_t h1 = vqmovn_u32(v1);
            uint16x4_t h2 = vqmovn_u32(v2);
            uint16x4_t h3 = vqmovn_u32(v3);

            uint8x8_t bytes0 = vqmovn_u16(vcombine_u16(h0, h1));
            uint8x8_t bytes1 = vqmovn_u16(vcombine_u16(h2, h3));

            uint8x16_t result = vcombine_u8(bytes0, bytes1);

            vst1q_u8(Y_Channel + i, result);
        }
        /**************** Cb now ****************/ 
        {    
            float32x4_t vRedCoeff = vdupq_n_f32(Coeff_Cb_R);
            float32x4_t vGrnCoeff = vdupq_n_f32(Coeff_Cb_G);
            float32x4_t vBluCoeff = vdupq_n_f32(Coeff_Cb_B);

            float32x4_t vOutput;
            vOutput = vmulq_f32(r32_0f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_0f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_0f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v0 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_1f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_1f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_1f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v1 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_2f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_2f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_2f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v2 = vcvtq_u32_f32(vOutput);
            
            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_3f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_3f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_3f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v3 = vcvtq_u32_f32(vOutput);

            uint16x4_t h0 = vqmovn_u32(v0);
            uint16x4_t h1 = vqmovn_u32(v1);
            uint16x4_t h2 = vqmovn_u32(v2);
            uint16x4_t h3 = vqmovn_u32(v3);

            uint8x8_t bytes0 = vqmovn_u16(vcombine_u16(h0, h1));
            uint8x8_t bytes1 = vqmovn_u16(vcombine_u16(h2, h3));

            uint8x16_t result = vcombine_u8(bytes0, bytes1);

            vst1q_u8(Cb_Channel + i, result);
        }
        /**************** Cr finally ****************/ 
        {    
            float32x4_t vRedCoeff = vdupq_n_f32(Coeff_Cr_R);
            float32x4_t vGrnCoeff = vdupq_n_f32(Coeff_Cr_G);
            float32x4_t vBluCoeff = vdupq_n_f32(Coeff_Cr_B);

            float32x4_t vOutput;
            vOutput = vmulq_f32(r32_0f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_0f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_0f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v0 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_1f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_1f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_1f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v1 = vcvtq_u32_f32(vOutput);

            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_2f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_2f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_2f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v2 = vcvtq_u32_f32(vOutput);
            
            // vOutput = vdupq_n_f32(0.0f);
            vOutput = vmulq_f32(r32_3f, vRedCoeff);
            vOutput = vmlaq_f32(vOutput, g32_3f, vGrnCoeff);
            vOutput = vmlaq_f32(vOutput, b32_3f, vBluCoeff);

            vOutput = vaddq_f32(vOutput , vdupq_n_f32(128.5f));
            vOutput = vmaxq_f32(vOutput , vdupq_n_f32(0.0f));
            vOutput = vminq_f32(vOutput , vdupq_n_f32(255.0f));
            uint32x4_t v3 = vcvtq_u32_f32(vOutput);

            uint16x4_t h0 = vqmovn_u32(v0);
            uint16x4_t h1 = vqmovn_u32(v1);
            uint16x4_t h2 = vqmovn_u32(v2);
            uint16x4_t h3 = vqmovn_u32(v3);

            uint8x8_t bytes0 = vqmovn_u16(vcombine_u16(h0, h1));
            uint8x8_t bytes1 = vqmovn_u16(vcombine_u16(h2, h3));

            uint8x16_t result = vcombine_u8(bytes0, bytes1);

            vst1q_u8(Cr_Channel + i, result);
        }
    }
    return 0;
}

uint32_t DownSampling(uint8_t *Cb_Channel, uint8_t *Cr_Channel, uint16_t width, uint16_t height, uint8_t *Cb_Out, uint8_t *Cr_Out)
{
    assert(Cb_Channel != NULL);
    assert(Cr_Channel != NULL);
    assert(Cb_Out != NULL);
    assert(Cr_Out != NULL);
    assert((width % 2) == 0);
    assert((height % 2) == 0);
    
    const uint16_t out_width = width / 2;

    for (uint16_t i = 0; i < height; i+=2) {
        for (uint16_t j = 0; j < width; j+=2) {
            const uint16_t out_index = (j / 2) + (i / 2) * out_width;
            uint16_t Cb_Acc = (uint16_t) Cb_Channel[(j + 0) + (i + 0) *  width] +  (uint16_t)  Cb_Channel[(j + 1) + (i + 0) *  width];
            Cb_Acc += Cb_Channel[(j + 0) + (i + 1) *  width] + Cb_Channel[(j + 1) + (i + 1) *  width];
            Cb_Acc += 2;
            Cb_Acc >>= 2;
            uint16_t Cr_Acc = (uint16_t) Cr_Channel[(j + 0) + (i + 0) *  width] +  (uint16_t)  Cr_Channel[(j + 1) + (i + 0) *  width];
            Cr_Acc += Cr_Channel[(j + 0) + (i + 1) *  width] + Cr_Channel[(j + 1) + (i + 1) *  width];
            Cr_Acc += 2;
            Cr_Acc >>= 2;
            Cr_Out[out_index] = (uint8_t) Cr_Acc;
            Cb_Out[out_index] = (uint8_t) Cb_Acc;

        }
    }
    return 0;
}

Padding PaddImage(uint8_t *Src_Channel, uint16_t width, uint16_t height, ChType type, uint8_t *Out_Channel)
{
    uint16_t width_div = width / 16;
    uint16_t width_rem = width % 16;

    uint16_t height_div = height / 16;
    uint16_t height_rem = height % 16;

    if (width_rem != 0) {
        for (uint16_t i = 0; i < height; i++) {
            for (uint16_t i = 0; i < width; i+=16) {
                
            }

        }
        
        return WIDTH_PADDED;
    } else if (height_rem != 0) {

        return HEIGHT_PADDED;
    }
    return PADDED_ALREADY;
}
