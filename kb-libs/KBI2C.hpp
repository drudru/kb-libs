


#pragma once

#include "common.h"

#include <sys/ioctl.h>
#include "i2c-dev.h"


// KBI2C
//
// Linux frame buffer object
//

struct KBI2C
{
    NXCanvas * canvas;
    int        i2cfd;

    KBI2C(NXRect screen_rect, NXColorChan chans)
    {
        i2cfd = open("/dev/i2c-1", O_RDWR);

        if (i2cfd == -1)
        {
            DBG_WRITE("Could not open /dev/i2c-1");
            panic();
        }
        if (ioctl(i2cfd, I2C_SLAVE, 0x3c) == -1)
        {
            DBG_WRITE("Could not set i2c address");
            exit(1);
        }

        // initialise the display
        //U8 initSequence[25] = {0xAE,0xA8,0x3F,0xD3,0x00,0x40,0xA1,0xC8,0xDA,0x12,0x81,0x7F,
        //0xA4,0xA6,0xD5,0x80,0x8D,0x14,0xD9,0x22,0xD8,0x30,0x20,0x00,0xAF};

        // From Adafruit
        U8 initSequence[25] = {
            0xAE,
            0xD5, 0x80,
            0xA8, 0x3F,
            0xD3, 0x00,
            0x40 | 0,
            0x8D, 0x14, // Chargepump, non external
            0x20, 0x00,
            0xA0 | 1,
            0xC8,
            0xDA, 0x12,
            0x81, 0x7F,
            0xD9, 0xF1, // Precharge, non external
            0xDB, 0x40,
            0xA4,
            0xA6 | 0,   // 0 = Normal, 1 = Inverse
            0xAF
        };

        write_cmd(initSequence, 25);


        int screen_datasize = screen_rect.size.w * screen_rect.size.h * chans;
        void * fbp = calloc(1, screen_datasize);

        canvas = new NXCanvas{ NXBitmap{(uint8_t *)fbp, screen_rect, chans} };
    }



    void write_cmd(U8 * data, int bytes)
    {
        if (bytes <= 16)
        {
            int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x00, bytes, data);
            if (res)
                DBG_WRITE("Bad i2c_smbus_write_i2c_block_data 1");
        }
        else
        {
            int start = 0;
            int num_left = bytes;

            while (num_left > 0)
            {
                int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x00,
                        num_left >= 16 ? 16 : num_left, data + start);
                if (res)
                    DBG_WRITE("Bad i2c_smbus_write_i2c_block_data 2");
                start += 16;
                num_left -= 16;
            }
        }
    }

    void write_data(U8 * data, int bytes)
    {
        if (bytes <= 16)
        {
            int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x40, bytes, data);
            if (res)
                DBG_WRITE("Bad i2c_smbus_write_i2c_block_data D1");
        }
        else
        {
            int start = 0;
            int num_left = bytes;

            while (num_left > 0)
            {
                int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x40,
                        num_left >= 16 ? 16 : num_left, data + start);
                if (res)
                    DBG_WRITE("Bad i2c_smbus_write_i2c_block_data D2");
                start += 16;
                num_left -= 16;
            }
        }
    }

    void dbg_render_fb(NXCanvas * canvas)
    {
        NXPoint pt;
        for (int row = 0; row < 64; row++)
        {
            pt.y = row;
            for (int col = 0; col < 128; col++)
            {
                pt.x = col;
                if ((canvas->get_pixel(pt)).is_black() == false)
                    write(1, "X", 1);
                else
                    write(1, ".", 1);
            }
            write(1, "\n", 1);
        }
    }

    void render()
    {
        dbg_render_fb(canvas);
        U8 buff[128];

        // Set the col and row "address"
        unsigned char setFullRange[6] = {0x21,0x00,0x7F,0x22,0x00,0x07};

        // 8 segments (groups of rows)
        for (int seg = 0; seg < 8; seg++)
        {
            setFullRange[4] = seg;
            write_cmd(setFullRange, 6);

            NXPoint pt;
            for (int col = 0; col < 128; col++)
            {
                U8 byte = 0;
                pt.x = col;

                for (int row = 7; row >= 0; row--)
                {
                    pt.y = (seg * 8) + row;
                    if ((canvas->get_pixel(pt)).is_black() == false)
                    {
                        byte |= 1;
                    }

                    byte <<= 1;
                }

                buff[col] = byte;
            }

            write_data(buff, 128);
        }
    }

};
