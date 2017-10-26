
#pragma once

#include "common.h"

#include "NXCanvas.hpp"
#include "KBScreen.hpp"

#include "NXProtoIterator.hpp"
#include "NXUnixPacketSocket.hpp"

struct KBListBox
{
    KBScreen           * _screen     = nullptr;
    NXCanvas           * _canvas     = nullptr;
    NXUnixPacketSocket * _events     = nullptr;
    NXRect               _text_rect;

private:
    U8 curr_choice;        
    U8 last_choice;        
public:

    KBListBox(KBScreen * screen, NXUnixPacketSocket * evts)
    {
        _screen = screen;
        _canvas = _screen->canvas();
        _events = evts;
    }

    ~KBListBox()
    {
        // TODO: fix leaks
    }

    void draw_choices(NXProtoIterator<char *> * pchoices)
    {
        U8 index = 0;


        NXColor prev_fg   = _canvas->state.fg;
        NXColor prev_bg   = _canvas->state.bg;
        _canvas->state.fg = prev_fg;
        _canvas->state.bg = prev_bg;

        NXPoint pt = {0, 0};
        pt.x = (_text_rect.origin.x) * _screen->font.char_size.w;
        pt.y = (_text_rect.origin.y) * _screen->font.char_size.h;

        fprintf(stderr, "draw_choices\n");

        pchoices->rewind();

        char * choice = pchoices->get_next();
        while (choice)
        {
            if (index == curr_choice)
            {
                // Swap colors
                _canvas->state.fg = prev_bg;
                _canvas->state.bg = prev_fg;
            }

            _canvas->draw_font(&_screen->font, pt, choice);

            if (index == curr_choice)
            {
                // Restore normal colors
                _canvas->state.fg = prev_fg;
                _canvas->state.bg = prev_bg;
            }

            pt.y += _screen->font.char_size.h;
            index++;
            choice = pchoices->get_next();
        }

        last_choice = index - 1;

        _canvas->state.fg = prev_fg;
        _canvas->state.bg = prev_bg;
    }

    int go (NXProtoIterator<char *> * pchoices, bool allow_cancel, int timeout)
    {
        while (true)
        {
            curr_choice = 0;
            last_choice = 0;
            draw_choices(pchoices);

            _screen->flush();

            // event loop
            while (true)
            {
                _events->send_msg("wait");
                auto msg = _events->recv_msg();

                // TODO: if debug
                //fprintf(stderr, "KBListBox msg %s\n", msg._str);
                if (msg == "up")
                {
                    if (curr_choice != 0)
                        curr_choice--;
                }
                else
                if (msg == "down")
                {
                    if (curr_choice != last_choice)
                        curr_choice++;
                }
                else
                if (msg == "left")
                {
                    if (allow_cancel)
                        return -1;
                }
                else
                if (msg == "select")
                {
                    return curr_choice;
                }
                else
                if (msg == "wake")
                {
                    // redraw screen
                    break;
                }
                else
                if (msg == "tick")
                {
                    timeout--;
                    if (timeout < 0)
                        return -1;
                    continue;
                }
                else
                if (msg == "")
                {
                    // kb-gui died, restart
                    usleep(200000);
                    exit(1);
                    break;
                }
                else
                {
                    fprintf(stderr, "KBListBox unhandled msg\n");
                    continue;
                }

                draw_choices(pchoices);
                _screen->flush();
            } // event loop

        } // draw menu loop
        
        // Return cancel
        return -1;
    }
};
