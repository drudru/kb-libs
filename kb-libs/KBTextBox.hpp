
#pragma once

#include "common.h"

#include "KBScreen.hpp"
#include "NXStringTokenizer.hpp"

struct KBTextBox
{
    KBScreen           * _screen     = nullptr;
    NXCanvas           * _canvas     = nullptr;
    const char         * _mesg       = nullptr;
    NXRect               _text_rect;


    KBTextBox(KBScreen * screen, const char * message)
    {
        _screen = screen;
        _canvas = screen->canvas();
        _mesg   = message;
    }

    ~KBTextBox()
    {
        // TODO: fix leaks
    }

    void draw()
    {
        NXPoint pt = {0, 0};
        pt.x = _text_rect.origin.x * _screen->font.char_size.w;
        pt.y = _text_rect.origin.y * _screen->font.char_size.h;

        NXStringTokenizer tokenizer{_mesg};

        // TODO: handle strings that are too long

        // TODO: add a clipping rect

        int curx = 0;
        while ( ! tokenizer.is_done() )
        {
            auto token = tokenizer.get_next();

            if (token._len > (_text_rect.size.w - curx))
            {
                // Next line
                pt.x = _text_rect.origin.x * _screen->font.char_size.w;
                pt.y = pt.y + _screen->font.char_size.h;
                curx = 0;
            }

            _canvas->draw_font(&_screen->font, pt, &token);
            curx += token._len + 1; // Virtual space appended
            pt.x += (token._len + 1) * _screen->font.char_size.w; 
        }
    }
};
