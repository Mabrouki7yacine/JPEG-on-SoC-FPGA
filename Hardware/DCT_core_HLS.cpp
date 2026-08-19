#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

#define INPUT_WIDTH            12
#define COEFF_WIDTH            13
#define OUTPUT_WIDTH           12
#define DCT_SHIFT              12

#define QUANT_RECIP_WIDTH       9
#define QUANT_OUTPUT_WIDTH      8
#define QUANT_SHIFT            12

#define BLOCKS_PER_GROUP        6
#define LUMA_BLOCKS             4

typedef ap_axiu<64, 0, 0, 0> axis64_t;

typedef ap_int<INPUT_WIDTH + COEFF_WIDTH>     mult_t;
typedef ap_int<INPUT_WIDTH + COEFF_WIDTH + 1> add0_t;
typedef ap_int<INPUT_WIDTH + COEFF_WIDTH + 2> add1_t;
typedef ap_int<INPUT_WIDTH + COEFF_WIDTH + 3> sum_t;
typedef ap_int<INPUT_WIDTH + COEFF_WIDTH + 4> wide_t;


const ap_int<COEFF_WIDTH> DCT_COEFF[8][8] = {
    {1448,  1448,  1448,  1448,  1448,  1448,  1448,  1448},
    {2009,  1703,  1138,   400,  -400, -1138, -1703, -2009},
    {1892,   784,  -784, -1892, -1892,  -784,   784,  1892},
    {1703,  -400, -2009, -1138,  1138,  2009,   400, -1703},
    {1448, -1448, -1448,  1448,  1448, -1448, -1448,  1448},
    {1138, -2009,   400,  1703, -1703,  -400,  2009, -1138},
    { 784, -1892,  1892,  -784,  -784,  1892, -1892,   784},
    { 400, -1138,  1703, -2009,  2009, -1703,  1138,  -400}
};


const ap_uint<QUANT_RECIP_WIDTH> LUMA_QUANT_RECIP[8][8] = {
    {256, 372, 410, 256, 171, 102,  80,  67},
    {341, 341, 293, 216, 158,  71,  68,  74},
    {293, 315, 256, 171, 102,  72,  59,  73},
    {293, 241, 186, 141,  80,  47,  51,  66},
    {228, 186, 111,  73,  60,  38,  40,  53},
    {171, 117,  74,  64,  51,  39,  36,  45},
    { 84,  64,  53,  47,  40,  34,  34,  41},
    { 57,  45,  43,  42,  37,  41,  40,  41}
};


const ap_uint<QUANT_RECIP_WIDTH> CHROMA_QUANT_RECIP[8][8] = {
    {241, 228, 171,  87, 41, 41, 41, 41},
    {228, 195, 158,  62, 41, 41, 41, 41},
    {171, 158,  73,  41, 41, 41, 41, 41},
    { 87,  62,  41,  41, 41, 41, 41, 41},
    { 41,  41,  41,  41, 41, 41, 41, 41},
    { 41,  41,  41,  41, 41, 41, 41, 41},
    { 41,  41,  41,  41, 41, 41, 41, 41},
    { 41,  41,  41,  41, 41, 41, 41, 41}
};


void multiplication_stage_8(
    const ap_int<INPUT_WIDTH> input[8],
    const ap_int<COEFF_WIDTH> coeff[8],
    mult_t result[8])
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1
#pragma HLS LATENCY min=1 max=1

#pragma HLS ARRAY_PARTITION variable=input complete
#pragma HLS ARRAY_PARTITION variable=coeff complete
#pragma HLS ARRAY_PARTITION variable=result complete

    for (int i = 0; i < 8; i++)
    {
#pragma HLS UNROLL
        result[i] = input[i] * coeff[i];
    }
}


void adder_tree_stage_8(
    const mult_t input[8],
    sum_t &result)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1
#pragma HLS LATENCY min=1 max=1

#pragma HLS ARRAY_PARTITION variable=input complete

    add0_t level0[4];
    add1_t level1[2];

#pragma HLS ARRAY_PARTITION variable=level0 complete
#pragma HLS ARRAY_PARTITION variable=level1 complete

    for (int i = 0; i < 4; i++)
    {
#pragma HLS UNROLL
        level0[i] = input[2 * i] + input[2 * i + 1];
    }

    level1[0] = level0[0] + level0[1];
    level1[1] = level0[2] + level0[3];
    result = level1[0] + level1[1];
}


void round_shift_stage(
    const sum_t input,
    ap_int<OUTPUT_WIDTH> &output)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1
#pragma HLS LATENCY min=1 max=1

    const wide_t half = wide_t(1) << (DCT_SHIFT - 1);
    wide_t value = input;
    wide_t shifted_result;
    if (value >= 0) {
        shifted_result = (value + half) >> DCT_SHIFT;
    } else {
        shifted_result = -(((-value) + half) >> DCT_SHIFT);
    }
    output = (ap_int<OUTPUT_WIDTH>)shifted_result;
}


void vector_mult_8(
    const ap_int<INPUT_WIDTH> input[8],
    const ap_int<COEFF_WIDTH> coeff[8],
    ap_int<OUTPUT_WIDTH> &output)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1

#pragma HLS ARRAY_PARTITION variable=input complete
#pragma HLS ARRAY_PARTITION variable=coeff complete

    mult_t multiplication_result[8];
    sum_t adder_tree_result;

#pragma HLS ARRAY_PARTITION variable=multiplication_result complete

    multiplication_stage_8( input, coeff, multiplication_result);
    adder_tree_stage_8( multiplication_result, adder_tree_result);
    round_shift_stage(adder_tree_result,output);
}


void vector_compute_8(
    const ap_int<INPUT_WIDTH> input[8],
    const ap_int<COEFF_WIDTH> coeff[8][8],
    ap_int<OUTPUT_WIDTH> output[8])
{
#pragma HLS INLINE off

#pragma HLS ARRAY_PARTITION variable=input complete
#pragma HLS ARRAY_PARTITION variable=output complete
#pragma HLS ARRAY_PARTITION variable=coeff complete dim=0

    for (int i = 0; i < 8; i++)
    {
#pragma HLS UNROLL
        vector_mult_8(input, coeff[i], output[i]);
    }
}


void quant_vector_8(
    const ap_int<OUTPUT_WIDTH> dct_input[8],
    const ap_uint<QUANT_RECIP_WIDTH> recip[8],
    ap_int<QUANT_OUTPUT_WIDTH> quant_output[8])
{
#pragma HLS INLINE off

#pragma HLS ARRAY_PARTITION variable=dct_input complete
#pragma HLS ARRAY_PARTITION variable=recip complete
#pragma HLS ARRAY_PARTITION variable=quant_output complete

    typedef ap_int<QUANT_RECIP_WIDTH + 1> signed_recip_t;
    typedef ap_int<OUTPUT_WIDTH + QUANT_RECIP_WIDTH + 1> product_t;
    typedef ap_int<OUTPUT_WIDTH + QUANT_RECIP_WIDTH + 2> quant_wide_t;

    product_t products[8];

#pragma HLS ARRAY_PARTITION variable=products complete

    for (int i = 0; i < 8; i++)
    {
#pragma HLS UNROLL
        signed_recip_t signed_recip = (signed_recip_t)recip[i];
        products[i] = dct_input[i] * signed_recip;
    }

    const quant_wide_t half = quant_wide_t(1) << (QUANT_SHIFT - 1);

    for (int i = 0; i < 8; i++)
    {
#pragma HLS UNROLL
        quant_wide_t value = products[i];
        quant_wide_t shifted_result;

        if (value >= 0){
            shifted_result = (value + half) >> QUANT_SHIFT;
        } else {
            shifted_result = -(((-value) + half) >> QUANT_SHIFT);
        }

        quant_output[i] = (ap_int<QUANT_OUTPUT_WIDTH>)shifted_result;
    }
}


void my_core(
    hls::stream<axis64_t> &s_axis,
    hls::stream<axis64_t> &m_axis)
{
#pragma HLS INTERFACE axis port=s_axis
#pragma HLS INTERFACE axis port=m_axis
#pragma HLS INTERFACE ap_ctrl_none port=return

    ap_int<INPUT_WIDTH> centered[8];

    ap_int<OUTPUT_WIDTH> dct1_output[8];

    ap_int<INPUT_WIDTH> second_input[8];
    ap_int<OUTPUT_WIDTH> dct2_output[8];

    ap_uint<QUANT_RECIP_WIDTH> selected_recip[8];
    ap_int<QUANT_OUTPUT_WIDTH> quant_output[8];

    ap_int<OUTPUT_WIDTH> buffer[8][8];

#pragma HLS ARRAY_PARTITION variable=centered complete
#pragma HLS ARRAY_PARTITION variable=dct1_output complete
#pragma HLS ARRAY_PARTITION variable=second_input complete
#pragma HLS ARRAY_PARTITION variable=dct2_output complete
#pragma HLS ARRAY_PARTITION variable=selected_recip complete
#pragma HLS ARRAY_PARTITION variable=quant_output complete
#pragma HLS ARRAY_PARTITION variable=buffer complete dim=0

    while (1)
    {
        for (int block = 0; block < BLOCKS_PER_GROUP; block++)
        {
            bool is_luma = block < LUMA_BLOCKS;

            for (int row = 0; row < 8; row++)
            {
                axis64_t in_word = s_axis.read();

                for (int col = 0; col < 8; col++)
                {
#pragma HLS UNROLL
                    ap_uint<8> current_pixel =in_word.data.range(8 * col + 7, 8 * col);

                    centered[col] = (ap_int<INPUT_WIDTH>)current_pixel -128;
                }

                vector_compute_8(centered, DCT_COEFF, dct1_output);

                for (int frequency = 0; frequency < 8; frequency++)
                {
#pragma HLS UNROLL
                    buffer[row][frequency] = dct1_output[frequency];
                }
            }

            for (int u = 0; u < 8; u++)
            {
                for (int row = 0; row < 8; row++)
                {
#pragma HLS UNROLL
                    second_input[row] = buffer[row][u];
                }

                vector_compute_8(second_input, DCT_COEFF, dct2_output);

                for (int v = 0; v < 8; v++)
                {
#pragma HLS UNROLL
                    buffer[v][u] = dct2_output[v];
                }
            }

            for (int row = 0; row < 8; row++)
            {
                for (int col = 0; col < 8; col++)
                {
#pragma HLS UNROLL
                    selected_recip[col] = is_luma ? LUMA_QUANT_RECIP[row][col] : CHROMA_QUANT_RECIP[row][col];
                }

                quant_vector_8(buffer[row], selected_recip, quant_output);

                axis64_t out_word;

                out_word.data = 0;
                out_word.keep = 0xFF;
                out_word.strb = 0xFF;

                for (int col = 0; col < 8; col++)
                {
#pragma HLS UNROLL
                    ap_uint<8> packed_byte = (ap_uint<8>)quant_output[col];
                    out_word.data.range(8 * col + 7,8 * col) = packed_byte;
                }
                out_word.last = (block == BLOCKS_PER_GROUP - 1) && (row == 7);
                m_axis.write(out_word);
            }
        }
    }
}