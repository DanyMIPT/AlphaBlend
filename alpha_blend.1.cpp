#include <iostream>
#include "D:\\TX\TXlib.h"

constexpr size_t WIDTH  = 800;
constexpr size_t HEIGHT = 600;

typedef unsigned char limpidity;
typedef unsigned char ONE_BYTE;
typedef RGBQUAD (&vid_memory) [HEIGHT][WIDTH];

constexpr const char* background = "ded.bmp";
constexpr const char* target     = "racket.bmp";

class Mem
{
public:
    vid_memory mem;

    Mem (const char* fileName);
    ~Mem ();

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
    auto vid_mem    = (vid_memory) *txVideoMemory ();

    for (size_t i = 0; i < 100;i++)
    {
        for (size_t y = 0; y < WIDTH; y++)
        {
            for (size_t x = 0; x < HEIGHT; x++)
            {
                RGBQUAD* fr = &target_mem.mem[x][y];
                RGBQUAD* bk = &back_mem.mem[x][y];

                limpidity factor = fr->rgbReserved;

                vid_mem[x][y] = {(ONE_BYTE) ((fr->rgbBlue  * factor + bk->rgbBlue  * (255 - factor)) >> 8),
                                 (ONE_BYTE) ((fr->rgbGreen * factor + bk->rgbGreen * (255 - factor)) >> 8),
                                 (ONE_BYTE) ((fr->rgbRed   * factor + bk->rgbRed   * (255 - factor)) >> 8)};
            }
        }
        txSleep ();
    }
}

vid_memory Mem::NoTxLoadImage (const char* fileName)
{
    RGBQUAD* img_mem = nullptr;
    lay = txCreateDIBSection (WIDTH, HEIGHT, &img_mem);

    HDC img = txLoadImage (fileName);
    if (!img)
        txMessageBox ("Image was not downloaded");

    txBitBlt (lay, (txGetExtentX (lay) - txGetExtentX (img)) / 2,
                   (txGetExtentY (lay) - txGetExtentY (img)) / 2, 0, 0, img);
    txDeleteDC (img);

    return (vid_memory) *img_mem;
}

Mem::Mem (const char* fileName): mem ((vid_memory) NoTxLoadImage (fileName)) {}

Mem::~Mem ()
{
    txDeleteDC (lay);
}
