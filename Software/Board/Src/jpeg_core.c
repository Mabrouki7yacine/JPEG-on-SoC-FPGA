#include "jpeg_core.h"

uint32_t RGB2YCbCr(const RGB* RGB_stream, const uint32_t block_size, uint8_t *Y_Channel, uint8_t *Cb_Channel, uint8_t *Cr_Channel)
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
    
    const uint32_t out_width = width / 2;

    for (uint16_t i = 0; i < height; i+=2) {
        for (uint16_t j = 0; j < width; j+=2) {
            const uint32_t out_index = (j / 2) + (i / 2) * out_width;
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

uint32_t GetNumBlocks8x8(uint32_t width, uint32_t height)
{
    uint32_t blocks_x = (width  + 7) / 8;
    uint32_t blocks_y = (height + 7) / 8;

    return blocks_x * blocks_y;
}

uint32_t GetNumBlocks16x16(uint32_t width, uint32_t height)
{
    uint32_t blocks_x = (width  + 15) / 16;
    uint32_t blocks_y = (height + 15) / 16;

    return blocks_x * blocks_y;
}

Padding_t PaddImage(const uint8_t *Src_Channel, uint16_t width, uint16_t height, ChType type, uint8_t *Out_Channel ) 
{
    assert(Src_Channel != NULL);
    assert(Out_Channel != NULL);
    assert(width > 0);
    assert(height > 0);
    assert(type == Luminance || type == Chroma);

    const uint16_t block = (uint16_t)type;

    const uint16_t width_rem  = width  % block;
    const uint16_t height_rem = height % block;

    if (width_rem == 0 && height_rem == 0) {
        return PADDED_ALREADY;
    }

    const uint16_t new_width  = (width_rem == 0) ? width : width + block - width_rem;
    const uint16_t new_height = (height_rem == 0) ? height : height + block - height_rem;

    /*
     * Copy every original row.
     * If width padding is required, repeat the final valid pixel.
     */
    for (uint16_t i = 0; i < height; i++) {
        const uint8_t *src_row = &Src_Channel[(size_t)i * width];
        uint8_t *dst_row = &Out_Channel[(size_t)i * new_width];

        uint16_t j = 0;

        /* Copy complete groups of 16 pixels using NEON. */
        for (; j + 15 < width; j += 16) {
            uint8x16_t pixels = vld1q_u8(&src_row[j]);
            vst1q_u8(&dst_row[j], pixels);
        }

        /* Copy remaining valid pixels. */
        for (; j < width; j++) {
            dst_row[j] = src_row[j];
        }

        /* Repeat the final valid pixel for right padding. */
        const uint8_t last_pixel = src_row[width - 1];

        for (; j < new_width; j++) {
            dst_row[j] = last_pixel;
        }
    }

    /*
     * If height padding is required, repeat the final padded row.
     */
    const uint8_t *last_row = &Out_Channel[(size_t)(height - 1) * new_width];

    for (uint16_t i = height; i < new_height; i++) {
        uint8_t *dst_row = &Out_Channel[(size_t)i * new_width];

        uint16_t j = 0;

        for (; j + 15 < new_width; j += 16) {
            uint8x16_t pixels = vld1q_u8(&last_row[j]);
            vst1q_u8(&dst_row[j], pixels);
        }

        for (; j < new_width; j++) {
            dst_row[j] = last_row[j];
        }
    }

    if (width_rem != 0 && height_rem != 0) {
        return REQUIRE_F_PADDING;
    }

    if (width_rem != 0) {
        return REQUIRE_W_PADDING;
    }

    return REQUIRE_H_PADDING;
}

uint32_t BuildMCU420(
    uint8_t *Y_Channel, 
    uint8_t *Cb_Channel, 
    uint8_t *Cr_Channel, 
    uint16_t width, 
    uint16_t height, 
    uMCU_block_t* MCU_block)
{
    assert(Y_Channel  != NULL);
    assert(Cb_Channel != NULL);
    assert(Cr_Channel != NULL);
    assert(MCU_block  != NULL);
    assert(width > 0);
    assert(height > 0);
    assert((width  % 16) == 0);
    assert((height % 16) == 0);

    uint16_t YwBlocks = width  / 16;
    uint16_t YhBlocks = height / 16;

    const uint16_t chroma_width = width / 2;
    // i hope no one will read this rah t3ya use python wla kch language fiha libs wajdin
    for (uint16_t i = 0; i < YhBlocks; i++){
        for (uint16_t j = 0; j < YwBlocks; j++){
            uint32_t MCU_Index = i * YwBlocks + j;

            for (uint16_t k = 0; k < 8; k++) {
                uint8x8_t  vSrc = vld1_u8( &(Y_Channel[(i * 16 + k) * width + (j * 16) ]) );
                vst1_u8(&(MCU_block[MCU_Index].Y0[k * 8]) , vSrc);

                vSrc = vld1_u8( &(Y_Channel[(i * 16 + k) * width + (j * 16) + 8]) );
                vst1_u8(&(MCU_block[MCU_Index].Y1[k * 8]) , vSrc);

                vSrc = vld1_u8( &(Y_Channel[(i * 16 + k) * width + (j * 16) + 8 * width]) );
                vst1_u8(&(MCU_block[MCU_Index].Y2[k * 8]) , vSrc);

                vSrc = vld1_u8( &(Y_Channel[(i * 16 + k) * width + (j * 16) + 8 * width + 8]) );
                vst1_u8(&(MCU_block[MCU_Index].Y3[k * 8]) , vSrc);

                vSrc = vld1_u8(&Cb_Channel[(i * 8 + k) * chroma_width +(j * 8)]);
                vst1_u8( &MCU_block[MCU_Index].Cb[k * 8], vSrc);

                vSrc = vld1_u8(&Cr_Channel[(i * 8 + k) * chroma_width +(j * 8)]);
                vst1_u8( &MCU_block[MCU_Index].Cr[k * 8], vSrc);
            }

            // MCU_block[MCU_Index].block_pos_width = j;
            // MCU_block[MCU_Index].block_pos_height = i;
        }
    }
    return YwBlocks * YhBlocks;
}

// static int wait_dma_done(int direction)
// {
//     int timeout = TIMEOUT_LIMIT;

//     while (XAxiDma_Busy(&AxiDma, direction)) {
//         timeout--;

//         if (timeout == 0) {
//             xil_printf("ERROR: DMA timeout\r\n");
//             return XST_FAILURE;
//         }
//     }

//     return XST_SUCCESS;
// }

int32_t SendBlockToPL(XAxiDma* AxiDma, uMCU_block_t* InMCU_block, sMCU_block_t* OutMCU_block)
{
    int32_t Status;
    Xil_DCacheFlushRange((UINTPTR)&InMCU_block->Y0[0] , BLOCK_SIZE);
    Xil_DCacheFlushRange((UINTPTR)&OutMCU_block->Y0[0], BLOCK_SIZE);

    Status = XAxiDma_SimpleTransfer(
        AxiDma,
        (UINTPTR)&OutMCU_block->Y0[0],
        BLOCK_SIZE,
        XAXIDMA_DEVICE_TO_DMA
    );

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: S2MM transfer failed to start\r\n");
        return Status;
    }

    Status = XAxiDma_SimpleTransfer(
        AxiDma,
        (UINTPTR)&InMCU_block->Y0[0],
        BLOCK_SIZE,
        XAXIDMA_DMA_TO_DEVICE
    );

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: MM2S transfer failed to start\r\n");
        return Status;
    }
    return XST_SUCCESS;
}

int32_t wait_dma_done(XAxiDma* AxiDma, int32_t direction)
{
    int timeout = TIMEOUT_LIMIT;

    while (XAxiDma_Busy(AxiDma, direction)) {
        timeout--;

        if (timeout == 0) {
            xil_printf("ERROR: DMA timeout\r\n");
            return XST_FAILURE;
        }
    }

    return XST_SUCCESS;
}

void DCDifferenceEncoding(const sMCU_block_t *QuantBlock, int16_t dc_diff_val[6], int reset)
{
    static int16_t previous_Y  = 0;
    static int16_t previous_Cb = 0;
    static int16_t previous_Cr = 0;

    if (reset == 1) {
        previous_Y  = 0;
        previous_Cb = 0;
        previous_Cr = 0;
    }

    int16_t current_dc;

    current_dc = QuantBlock->Y0[0];
    dc_diff_val[0] = current_dc - previous_Y;
    previous_Y = current_dc;

    current_dc = QuantBlock->Y1[0];
    dc_diff_val[1] = current_dc - previous_Y;
    previous_Y = current_dc;

    current_dc = QuantBlock->Y2[0];
    dc_diff_val[2] = current_dc - previous_Y;
    previous_Y = current_dc;

    current_dc = QuantBlock->Y3[0];
    dc_diff_val[3] = current_dc - previous_Y;
    previous_Y = current_dc;

    current_dc = QuantBlock->Cb[0];
    dc_diff_val[4] = current_dc - previous_Cb;
    previous_Cb = current_dc;

    current_dc = QuantBlock->Cr[0];
    dc_diff_val[5] = current_dc - previous_Cr;
    previous_Cr = current_dc;
}


void ZigZagScan(sMCU_block_t* QuantBlock, sMCU_block_t* ZigzagBlock)
{
    for (uint8_t i = 0; i < 64; i++) {
        ZigzagBlock->Y0[i] = QuantBlock->Y0[JPEG_ZIGZAG[i]];
        ZigzagBlock->Y1[i] = QuantBlock->Y1[JPEG_ZIGZAG[i]];
        ZigzagBlock->Y2[i] = QuantBlock->Y2[JPEG_ZIGZAG[i]];
        ZigzagBlock->Y3[i] = QuantBlock->Y3[JPEG_ZIGZAG[i]];
        ZigzagBlock->Cb[i] = QuantBlock->Cb[JPEG_ZIGZAG[i]];
        ZigzagBlock->Cr[i] = QuantBlock->Cr[JPEG_ZIGZAG[i]];
    }
}

static inline uint8_t GetCategoryInt8(int8_t value)
{
    return CATEGORY_LUT[(uint8_t)value];
}

static inline uint8_t GetCategoryInt16(int16_t value)
{
    return DC_CATEGORY_LUT[value + 255];
}

uint32_t RunLengthEncoding(const int8_t *QuantCoeff, RLE_Entry_t* RLE_Entry)
{
    int8_t  ZigZagValue = 0;
    uint8_t ZeroCount   = 0;
    uint8_t ZrlCount    = 0;
    uint8_t RLE_Entry_Count = 0;
    uint8_t bit_size;
    for (uint8_t i = 1; i < 64; i++) {
        ZigZagValue = QuantCoeff[JPEG_ZIGZAG[i]];
        if (ZigZagValue ==  0) {
            ZeroCount++;
            if (ZeroCount >= 16) {
                ZeroCount -= 16;
                ZrlCount++;
            }
            if (i == 63) {
                RLE_Entry[RLE_Entry_Count].symbol = EOB_VALUE; 
                RLE_Entry[RLE_Entry_Count].value  = 0x00;
                RLE_Entry_Count++;
            }
        } else {
            for (int j = 0; j < ZrlCount; j++) {
                RLE_Entry[RLE_Entry_Count].symbol = ZRL_VALUE; 
                RLE_Entry[RLE_Entry_Count].value  = 0x00; 
                RLE_Entry_Count++;
            }
            ZrlCount = 0;

            bit_size = GetCategoryInt8(ZigZagValue);
            RLE_Entry[RLE_Entry_Count].symbol = (ZeroCount << 4) | bit_size;
            RLE_Entry[RLE_Entry_Count].value  = ZigZagValue;
            RLE_Entry_Count++;

            ZeroCount = 0;
        }
    }
    return RLE_Entry_Count;
}

void DCDiffEnc_ZigZag_RLE(
    const sMCU_block_t *QuantBlock,
    HuffmanBlock_t *HuffmanBlock,
    int8_t *previousY,
    int8_t *previousCb,
    int8_t *previousCr)
{
    const int8_t *blocks[6] = {
        QuantBlock->Y0,
        QuantBlock->Y1,
        QuantBlock->Y2,
        QuantBlock->Y3,
        QuantBlock->Cb,
        QuantBlock->Cr
    };

    for (uint8_t block = 0; block < 6; block++)
    {
        int8_t ZigZagValue = 0;
        uint8_t ZeroCount = 0;
        uint8_t ZrlCount = 0;
        uint8_t RLE_Entry_Count = 0;
        uint8_t bit_size;
        int16_t current_dc;

        //DC DIFF
        current_dc = (int16_t)blocks[block][0];

        if (block < 4) {
            HuffmanBlock[block].DC = current_dc - (int16_t)(*previousY);
            *previousY = (int8_t)current_dc;
        }
        else if (block == 4) {
            HuffmanBlock[block].DC = current_dc - (int16_t)(*previousCb);
            *previousCb = (int8_t)current_dc;
        } else {
            HuffmanBlock[block].DC =  current_dc - (int16_t)(*previousCr);
            *previousCr = (int8_t)current_dc;
        }

        // ZIGZAG + RLE

        for (uint8_t i = 1; i < 64; i++)
        {
            ZigZagValue = blocks[block][JPEG_ZIGZAG[i]];

            if (ZigZagValue == 0) {
                ZeroCount++;
                if (ZeroCount >= 16) {
                    ZeroCount -= 16;
                    ZrlCount++;
                }
                if (i == 63) {
                    HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].symbol = EOB_VALUE;
                    HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].value = 0x00;
                    RLE_Entry_Count++;
                }
            } else {
                for (uint8_t j = 0; j < ZrlCount; j++)
                {
                    HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].symbol = ZRL_VALUE;
                    HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].value = 0x00;
                    RLE_Entry_Count++;
                }

                ZrlCount = 0;
                bit_size = GetCategoryInt8(ZigZagValue);
                HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].symbol = (ZeroCount << 4) | bit_size;
                HuffmanBlock[block].RLE_Entry[RLE_Entry_Count].value = ZigZagValue;
                RLE_Entry_Count++;
                ZeroCount = 0;
            }
        }
        HuffmanBlock[block].RLE_Entry_Count = RLE_Entry_Count;
    }
}

// Inchallah manzidch nkhrb f hadi l func
uint64_t HuffmanEncoding_ByteStuffing(const HuffmanBlock_t *HuffmanBlock, uint32_t NumBlocks, uint8_t* bitstream) {
    uint64_t BitCount = 0;
    for (uint32_t i = 0; i < NumBlocks; i++) {
        uint8_t DcCategory = GetCategoryInt16(HuffmanBlock[i].DC);
        uint16_t amplitude;

        if (HuffmanBlock[i].DC >= 0)
            amplitude = HuffmanBlock[i].DC;
        else
            amplitude = HuffmanBlock[i].DC + ((1U << DcCategory) - 1);

        HuffmanCode_t huff = ((i % 6) < 4) ? DC_LUMA_HUFFMAN[DcCategory] : DC_CHROMA_HUFFMAN[DcCategory];
        uint8_t BitsToWrite = DcCategory + huff.length;
        uint32_t Bits = (huff.code << DcCategory) | amplitude;
        uint32_t BytesWritten = 0;
        if (BitCount % 8 == 0) {
            if (BitsToWrite > 8) {
                uint8_t RemainingBits = BitsToWrite - 8;
                bitstream[BitCount / 8] = (Bits >> RemainingBits) & 0xFF;
                bitstream[(BitCount / 8) + 1] = (Bits & ((1U << RemainingBits) - 1)) << (8 - RemainingBits);        
                BytesWritten = 2;
            } else {
                uint8_t Shift = 8 - BitsToWrite; 
                bitstream[BitCount / 8] = (Bits & 0xFF) << Shift;
                BytesWritten = 1;
            }
        } else {
            if (BitsToWrite > 16 - (BitCount % 8)) {
                uint8_t BitWritten = 0;
                uint8_t BitWrite = 8 - (BitCount % 8);
                uint8_t Shift = BitsToWrite - BitWrite; 
                bitstream[(BitCount / 8)] |= ( (Bits >> Shift) & ((1U << BitWrite) - 1));

                BitWritten = BitWrite;
                bitstream[(BitCount / 8) + 1] = ((Bits >> (Shift - 8)) & 0xFF);

                BitWritten += 8;
                BitWrite = BitsToWrite - BitWritten;
                Shift = 8 - BitWrite; 
                bitstream[(BitCount / 8) + 2] = Bits << Shift;
                BytesWritten = 3;
            } else if (BitsToWrite > 8 - (BitCount % 8)) {
                uint8_t BitWrite = 8 - (BitCount % 8);
                uint8_t Shift = BitsToWrite - BitWrite; 
                bitstream[(BitCount / 8)] |= ( (Bits >> Shift) & ((1U << BitWrite) - 1));

                BitWrite = BitsToWrite - BitWrite;
                Shift = 8 - BitWrite; 
                bitstream[(BitCount / 8) + 1] = Bits << Shift;
                BytesWritten = 2;
            } else {
                uint8_t RemainingBits = 8 - BitsToWrite;
                uint8_t Shift = RemainingBits - (BitCount % 8); 
                bitstream[(BitCount / 8)] |= (Bits & ((1U << BitsToWrite) - 1)) << (Shift);
                BytesWritten = 1;
            }
        }

        uint8_t  FF_Shifts = 0;
        uint8_t  bitstream_temp[5] = {};
        uint64_t BaseIndex = BitCount / 8;

        for (uint8_t s = 0; s < BytesWritten; s++) {
            bitstream_temp[s] = bitstream[BaseIndex + s];
        }

        uint8_t index = 0;
        for (uint8_t c = 0; c < BytesWritten; c++) {
            bitstream[BaseIndex + index] =  bitstream_temp[c];
            index++;
            if (bitstream_temp[c] == 0xFF) {
                bitstream[BaseIndex + index] = 0x00;
                FF_Shifts++;
                index++;
            }
        }

        BitCount += BitsToWrite;
        BitCount += (FF_Shifts * 8);

        for (uint8_t j = 0; j < HuffmanBlock[i].RLE_Entry_Count; j++) {
            RLE_Entry_t RLE_Entry = HuffmanBlock[i].RLE_Entry[j];
            // so i should Huffman table lookup(symbol) and append amplitude bits from value then pack the bits to bitstream
            int8_t  Amplitude;
            uint8_t Symbol = RLE_Entry.symbol;
            uint8_t Category = GetCategoryInt8(RLE_Entry.value);

            if (RLE_Entry.value >= 0)
                Amplitude = RLE_Entry.value;
            else
                Amplitude = RLE_Entry.value + ((1U << Category) - 1);

            huff = ((i % 6) < 4) ? AC_LUMA_HUFFMAN[Symbol] : AC_CHROMA_HUFFMAN[Symbol];
            BitsToWrite = Category + huff.length;
            Bits = (huff.code << Category) | Amplitude;

            if (BitCount % 8 == 0) {
                if (BitsToWrite > 16) {
                    uint8_t RemainingBits = BitsToWrite - 8;
                    bitstream[BitCount / 8] = (Bits >> RemainingBits) & 0xFF;

                    RemainingBits -= 8;
                    bitstream[(BitCount / 8) + 1] = (Bits >> RemainingBits) & 0xFF;

                    bitstream[(BitCount / 8) + 2] = (Bits & ((1U << RemainingBits) - 1)) << (8 - RemainingBits);
                    BytesWritten = 3;
                } else if (BitsToWrite > 8) {
                    uint8_t RemainingBits = BitsToWrite - 8;
                    bitstream[BitCount / 8] = (Bits >> RemainingBits) & 0xFF;

                    bitstream[(BitCount / 8) + 1] = (Bits & ((1U << RemainingBits) - 1)) << (8 - RemainingBits);
                    BytesWritten = 2;
                } else {
                    uint8_t Shift = 8 - BitsToWrite; 
                    bitstream[BitCount / 8] = (Bits & 0xFF) << Shift;
                    BytesWritten = 1;
                }
            } else {
                if (BitsToWrite > 24 - (BitCount % 8)) {
                    uint8_t BitWritten = 0;
                    uint8_t BitWrite = 8 - (BitCount % 8);
                    uint8_t Shift = BitsToWrite - BitWrite; 
                    bitstream[(BitCount / 8)] |= ( (Bits >> Shift) & ((1U << BitWrite) - 1));
                    BitWritten = BitWrite;

                    Shift -= 8;
                    bitstream[(BitCount / 8) + 1] = ((Bits >> Shift) & 0xFF);
                    BitWritten += 8;

                    Shift -= 8;
                    bitstream[(BitCount / 8) + 2] = ((Bits >> Shift) & 0xFF);
                    BitWritten += 8;

                    BitWrite = BitsToWrite - BitWritten;
                    Shift = 8 - BitWrite; 
                    bitstream[(BitCount / 8) + 3] = Bits << Shift;
                    BytesWritten = 4;
                } else if (BitsToWrite > 16 - (BitCount % 8)) {
                    uint8_t BitWritten = 0;
                    uint8_t BitWrite = 8 - (BitCount % 8);
                    uint8_t Shift = BitsToWrite - BitWrite; 
                    bitstream[(BitCount / 8)] |= ( (Bits >> Shift) & ((1U << BitWrite) - 1));

                    BitWritten = BitWrite;
                    bitstream[(BitCount / 8) + 1] = ((Bits >> (Shift - 8)) & 0xFF);

                    BitWritten += 8;
                    BitWrite = BitsToWrite - BitWritten;
                    Shift = 8 - BitWrite; 
                    bitstream[(BitCount / 8) + 2] = Bits << Shift;
                    BytesWritten = 3;
                } else if (BitsToWrite > 8 - (BitCount % 8)) {
                    uint8_t BitWrite = 8 - (BitCount % 8);
                    uint8_t Shift = BitsToWrite - BitWrite; 
                    bitstream[(BitCount / 8)] |= ( (Bits >> Shift) & ((1U << BitWrite) - 1));

                    BitWrite = BitsToWrite - BitWrite;
                    Shift = 8 - BitWrite; 
                    bitstream[(BitCount / 8) + 1] = Bits << Shift;
                    BytesWritten = 2;
                } else {
                    uint8_t RemainingBits = 8 - BitsToWrite;
                    uint8_t Shift = RemainingBits - (BitCount % 8); 
                    bitstream[(BitCount / 8)] |= (Bits & ((1U << BitsToWrite) - 1)) << (Shift);
                    BytesWritten = 1;
                }
            }

            uint8_t  FF_Shifts = 0;
            uint8_t  bitstream_temp[5] = {};
            uint64_t BaseIndex = BitCount / 8;

            for (uint8_t s = 0; s < BytesWritten; s++) {
                bitstream_temp[s] = bitstream[BaseIndex + s];
            }

            uint8_t index = 0;
            for (uint8_t c = 0; c < BytesWritten; c++) {
                bitstream[BaseIndex + index] =  bitstream_temp[c];
                index++;
                if (bitstream_temp[c] == 0xFF) {
                    bitstream[BaseIndex + index] = 0x00;
                    FF_Shifts++;
                    index++;
                }
            }

            BitCount += BitsToWrite;
            BitCount += (FF_Shifts * 8);
        }
    }
    uint64_t RemainingBits = 8 - (BitCount % 8);

    if (RemainingBits != 8) {
        uint64_t LastByteIndex = BitCount / 8;

        uint8_t mask = (1U << RemainingBits) - 1;
        bitstream[LastByteIndex] |= mask;

        BitCount += RemainingBits;

        if (bitstream[LastByteIndex] == 0xFF) {
            bitstream[LastByteIndex + 1] = 0x00;
            BitCount += 8;
        }
    }
    
    return BitCount;
}