

#pragma once

#include "common.h"

#include "NXCanvas.hpp"

// KBFB
//
// Linux frame buffer object
//

struct KBFB
{
    NXCanvas canvas;

    KBFB()
    {
    }

    void init(NXRect screen_rect, NXColorChan chans)
    {
        int fbfd = open("/dev/fb1", O_RDWR);

        if (fbfd == -1)
        {
            DBG_WRITE("Could not open /dev/fb1");
            panic();
        }

        int screen_datasize = screen_rect.size.w * screen_rect.size.h * chans;
        void * fbp = mmap(0, screen_datasize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
        close(fbfd);

        canvas = NXCanvas{ NXBitmap{(uint8_t *)fbp, screen_rect, chans} };
    }
};


