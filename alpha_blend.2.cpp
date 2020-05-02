#include <iostream>
#include <immintrin.h>
#include "D:\\TX\TXlib.h"

constexpr size_t WIDTH  = 800;
constexpr size_t HEIGHT = 600;

constexpr char MAKE_SHUF_FLAG          = 128;
constexpr size_t Move_dst_first_pixel  = 14;
constexpr size_t Move_dst_second_pixel = 6;
constexpr char Max_uchar               = 255;

typedef RGBQUAD (&vid_memory) [HEIGHT][WIDTH];

constexpr char* background = "C:\\Users\\dunka\\CLionProjects\\alpha_blending\\cmake-build-debug\\ded.bmp";
constexpr char* target     = "C:\\Users\\dunka\\CLionProjects\\alpha_blending\\cmake-build-debug\\kot.bmp";
constexpr char* result     = "C:\\Users\\dunka\\CLionProjects\\alpha_blending\\cmake-build-debug\\result.bmp";

class Mem
{
public:
    vid_memory mem;

    Mem (const char* fileName):
        mem ((vid_memory) NoTxLoadImage (fileName)) { }


    ~Mem ()
    {
        txDeleteDC (lay);
    }

private:
    vid_memory NoTxLoadImage (const char* fileName);
    HDC lay;
};

int main ()
{
    txCreateWindow (WIDTH, HEIGHT);

    Mem back_mem   (background);
    Mem target_mem (target);

    txBegin ();
    auto vid_mem = (vid_memory) *txVideoMemory ();

    for (size_t i = 0; i < 1000;i++)
    {
        for (size_t y = 0; y < HEIGHT; y++)
        {
            for (size_t x = 0; x < WIDTH; x += 4)
            {
                //Put four pixels to 128bit registers
                __m128i target_2_1_pixels = _mm_load_si128 ((__m128i*) &target_mem.mem[y][x]);
                __m128i back_2_1_pixels   = _mm_load_si128 ((__m128i*) &back_mem.mem[y][x]);

                //Now target_4_3_pixels = {0, 0, 4 pixel, 3 pixel} of target
                //    back_4_3_pixels   = {0, 0, 4 pixel, 3 pixel} of back
                //So important parts of the target_2_1_pixels/back_2_1_pixels are 1-st ans 2-nd pixels
                ////Why compiler himself doesn't cast __m128i to __m128??????
                //_mm_movehl_ps moves the upper 2 single-precision (32-bit) floating-point elements from second argument
                // to the lower 2 elements of dst, and copy the upper 2 elements from first argument to the upper 2 elements of dst.
                const __m128i const_0  = _mm_set1_epi8 (0);

                auto target_4_3_pixels = (__m128i) _mm_movehl_ps ((__m128) const_0, (__m128) target_2_1_pixels);
                auto back_4_3_pixels   = (__m128i) _mm_movehl_ps ((__m128) const_0, (__m128) back_2_1_pixels);

                //Cast to 2 bytes all pixels
                //_mm_cvtepu8_epi16 makes two bytes int from one byte int
                target_2_1_pixels = _mm_cvtepu8_epi16 (target_2_1_pixels);
                back_2_1_pixels   = _mm_cvtepu8_epi16 (back_2_1_pixels);
                target_4_3_pixels = _mm_cvtepu8_epi16 (target_4_3_pixels);
                back_4_3_pixels   = _mm_cvtepu8_epi16 (back_4_3_pixels);

                //Make necessary limpidity to multiply
                //https://software.intel.com/sites/landingpage/IntrinsicsGuide/#expand=426,85,3863,85,1606,4965,4965,5205,5152&text=_mm_shuffle_epi8
                const __m128i move_mask = _mm_set_epi8 (MAKE_SHUF_FLAG, Move_dst_first_pixel,  MAKE_SHUF_FLAG, Move_dst_first_pixel,  MAKE_SHUF_FLAG, Move_dst_first_pixel,  MAKE_SHUF_FLAG, Move_dst_first_pixel,
                                                        MAKE_SHUF_FLAG, Move_dst_second_pixel, MAKE_SHUF_FLAG, Move_dst_second_pixel, MAKE_SHUF_FLAG, Move_dst_second_pixel, MAKE_SHUF_FLAG, Move_dst_second_pixel);

                __m128i factor1 = _mm_shuffle_epi8 (target_2_1_pixels, move_mask);
                __m128i factor2 = _mm_shuffle_epi8 (target_4_3_pixels, move_mask);

                //Multiply target_pixel by limpidity
                target_2_1_pixels = _mm_mullo_epi16 (target_2_1_pixels, factor1);
                target_4_3_pixels = _mm_mullo_epi16 (target_4_3_pixels, factor2);

                //Multiply back pixel by (255 - limpidity)
                //Where are my 1 bytes commands???
                //Again make 2 byte int from 1 byte
                const __m128i const_255 = _mm_cvtepu8_epi16 (_mm_set1_epi8 (Max_uchar));

                back_2_1_pixels = _mm_mullo_epi16 (back_2_1_pixels, _mm_sub_epi16 (const_255, factor1));
                back_4_3_pixels = _mm_mullo_epi16 (back_4_3_pixels, _mm_sub_epi16 (const_255, factor2));

                //Sum results
                __m128i sum_2_1_pixels =  _mm_add_epi16 (target_2_1_pixels, back_2_1_pixels);
                __m128i sum_4_3_pixels =  _mm_add_epi16 (target_4_3_pixels, back_4_3_pixels);

                //Make right position of bytes, see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#expand=426,85,3863,85,1606,4965,4965,5205,5152&text=_mm_shuffle_epi8
                //for better understanding. Also that site explains all other commands not in humanitarian way.
                //P.S. The digits below are used to move bytes on necessary places
                const __m128i sum_mask = _mm_set_epi8 (MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG, MAKE_SHUF_FLAG,
                                                       15,             13,             11,             9,              7,              5,              3,              1             );

                sum_2_1_pixels = _mm_shuffle_epi8 (sum_2_1_pixels, sum_mask);
                sum_4_3_pixels = _mm_shuffle_epi8 (sum_4_3_pixels, sum_mask);

                //Move result pixels to screen memory
                auto result_colour = (__m128i) _mm_movelh_ps ((__m128) sum_2_1_pixels, (__m128) sum_4_3_pixels);
                _mm_storeu_si128 ((__m128i*) &vid_mem[y][x], result_colour);
            }
        }
        txSleep ();
    }

    txEnd ();
    txSaveImage (result, txDC ());
}

vid_memory Mem::NoTxLoadImage (const char* fileName)
{
    RGBQUAD* img_mem = nullptr;
    lay = txCreateDIBSection (WIDTH, HEIGHT, &img_mem);

    HDC img = txLoadImage (fileName);
    if (!img)
    {
        txMessageBox ("Image was not downloaded, the program shut downs");
        exit (1);
    }

    txBitBlt (lay, (txGetExtentX (lay) - txGetExtentX (img)) / 2,
             (txGetExtentY (lay) - txGetExtentY (img)) / 2, 0, 0, img);
    txDeleteDC (img);

    return (vid_memory) *img_mem;
}
