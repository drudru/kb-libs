
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
    int                  _padding;

private:
    U8 top_offset;
    U8 curr_choice;        
    U8 last_choice;        
public:

    KBListBox(KBScreen * screen, NXUnixPacketSocket * evts)
    {
        _screen = screen;
        _canvas = _screen->canvas();
        _events = evts;
        _padding = 0;
    }

    ~KBListBox()
    {
        // TODO: fix leaks
    }

    int prep_choices(NXProtoIterator<char *> * pchoices)
    {
        int num_choices = 0;

        pchoices->rewind();

        while (pchoices->get_next())
            num_choices++;

        pchoices->rewind();

        int offset_count = top_offset;
        while (offset_count--)
            pchoices->get_next();

        return num_choices;
    }

    void draw_choices(NXProtoIterator<char *> * pchoices)
    {

        NXRect box;
        box.origin.x = (_text_rect.origin.x) * _screen->font.char_size.w;
        box.origin.y = (_text_rect.origin.y) * _screen->font.char_size.h;
        box.size.w   = (_text_rect.size.w)   * _screen->font.char_size.w;
        box.size.h   = (_text_rect.size.h)   * _screen->font.char_size.h;
        _canvas->fill_rect(&box, _canvas->state.bg);

        NXPoint pt = box.origin;
        pt.x += _padding;
        //pt.y += _padding;
        fprintf(stderr, "draw_choices\n");

        int num_choices = prep_choices(pchoices);
        last_choice = num_choices - 1;


        U8 index = 0;
        char * choice = pchoices->get_next();
        while (choice)
        {
            _canvas->draw_font(&_screen->font, pt, choice);

            pt.y += _screen->font.char_size.h;
            index++;
            if (index == _text_rect.size.h)
                break;
            choice = pchoices->get_next();
        }

        // Draw inverted rect for selection
        {
            NXRect selection_rect;
            selection_rect.origin.x = (_text_rect.origin.x) * _screen->font.char_size.w;
            selection_rect.origin.y = (_text_rect.origin.y + curr_choice - top_offset) * _screen->font.char_size.h;
            selection_rect.size.w = (_text_rect.size.w - 1) * _screen->font.char_size.w;
            selection_rect.size.h = _screen->font.char_size.h;

            _canvas->fill_rect(&selection_rect, _canvas->state.fg, true);
        }

        // Draw scroll indicators
        if ((top_offset != 0) ||
            ((top_offset + _text_rect.size.h) < (last_choice + 1)))
        {
            NXPoint pt;
            pt.x = (_text_rect.origin.x + _text_rect.size.w - 1) * _screen->font.char_size.w;
            pt.y = _text_rect.origin.y * _screen->font.char_size.h;

            if (top_offset)
                _canvas->draw_font(&_screen->font, pt, "\x18");

            pt.y += (_text_rect.size.h - 1) * _screen->font.char_size.h;
            if ((top_offset + _text_rect.size.h) < (last_choice + 1))
                _canvas->draw_font(&_screen->font, pt, "\x19");
        }
    }

    void move_selection(int offset)
    {
        if (offset == -1)
        {
            if (curr_choice == 0)
                return;
            curr_choice--;

            if ((curr_choice - top_offset) < 0)
            {
                top_offset--;
            }
        }
        else
        {
            if (curr_choice == last_choice)
                return;
            curr_choice++;
            if ((curr_choice - top_offset) >= _text_rect.size.h)
            {
                top_offset++;
            }

        }
    }

    int go (NXProtoIterator<char *> * pchoices, bool allow_cancel, int timeout)
    {
        bool has_timeout = (timeout > 0);

        while (true)
        {
            top_offset  = 0;
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
                    move_selection(-1);
                }
                else
                if (msg == "down")
                {
                    move_selection( 1);
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
                    if (!has_timeout)
                        continue;

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
