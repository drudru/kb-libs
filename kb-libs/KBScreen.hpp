

#pragma once

#include "common.h"
#include "KBFB.hpp"
#include "KBI2C.hpp"
#include "NXCanvas.hpp"
#include "NXFilePath.hpp"
#include "NXMmapFile.hpp"
#include "NXStringTokenizer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"


// KBScreen
//
// An abstraction over the screen for the KeyBox project
//
// This interface requires a 'flush' after canvas operations
//

enum KBDispType
{
    DT_PITFT_22,
    DT_ADA_OLED_BONNET,
    DT_END
};
    
static const char * KBDispTypeStr[] =
{
    "pitft-22",
    "ada-oled-bonnet"
};
    
struct KBScreen
{


    KBDispType    disp_type;
    NXRect        screen_rect;
    NXRect        text_rect;

    const char  * font_path;
    NXFontAtlas   font;

    KBFB          _fb;
    KBI2C         _i2c;

    void load_font();

    KBScreen()
    {
        disp_type = KBDispType::DT_PITFT_22;

        NXMmapFile mmap;
        if (mmap.map("/boot/kbdisp.txt"))
        {
            // Tokenize to avoid any newline issues
            NXStringTokenizer tokenizer{mmap.ptr()};

            auto token = tokenizer.get_next();

            for (int i = KBDispType::DT_PITFT_22; i != KBDispType::DT_END; i++)
                if (token == KBDispTypeStr[i])
                {
                    disp_type = (KBDispType)i;
                    break;
                }
        }


        switch (disp_type)
        {
            case DT_PITFT_22:
                {
                    screen_rect = {0, 0, 320, 240};
                    _fb.init(screen_rect, NXColorChan::RGB565);

                    // https://en.wikipedia.org/wiki/Video_Graphics_Array#Color_palette
                    _fb.canvas.state.fg = NXColor{ 255, 255,  85, 255}; // Yellow
                    _fb.canvas.state.bg = NXColor{   0,   0, 170, 255}; // Blue
                    _fb.canvas.state.mono_color_txform = true;

                    load_font();

                    text_rect = {0, 0, 53, 18};
                }
                break;
            case DT_ADA_OLED_BONNET:
                {
                    screen_rect = {0, 0, 128, 64};
                    _i2c.init(screen_rect, NXColorChan::GREY1); // Really mono
                    _i2c.canvas.state.mono_color_txform = true;

                    load_font();

                    text_rect = {0, 0, 21, 5};
                }

                break;
            default:
                panic();    // This should never happen
        }
    }


    NXCanvas * canvas()
    {
        switch (disp_type)
        {
            case DT_PITFT_22:
                return &_fb.canvas;
            case DT_ADA_OLED_BONNET:
                return &_i2c.canvas;
        }
    }

    void flush()
    {
        switch (disp_type)
        {
            case DT_PITFT_22:
                // nop
                break;
            case DT_ADA_OLED_BONNET:
                _i2c.render();
                break;
        }
    }
};

void KBScreen::load_font()
{
    font_path = "/boot/KeyBox/tewi-11.png";

    int stb_width, stb_height, stb_bpp;
    unsigned char* font_stb_bmp = stbi_load(font_path, &stb_width, &stb_height, &stb_bpp, 1 );
    if (!font_stb_bmp)
    {
        fprintf(stderr, "missing font file %s\n", font_path);
        exit(1);
    }
    int16_t width  = stb_width;
    int16_t height = stb_height;
    int8_t  chans  = stb_bpp;
    NXBitmap * font_bmp =  new NXBitmap { (uint8_t *)font_stb_bmp, {0, 0, width, height}, NXColorChan::GREY1 };
    //fprintf(stderr, "font width: %d height: %d chans: %d\n", width, height, chans);

    //font = NXFontAtlas();
    font.atlas = font_bmp;
    font.rect  = { { 0, 0 }, { 192, 104 } };
    font.size  = { 32, 8 };
    font.init();
}
