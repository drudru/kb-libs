


#pragma once

#include "common.h"

// KBFB
//
// Linux frame buffer object
//

struct KBFB
{
    NXCanvas * canvas;

    KBFB(NXRect * screen_rect, NXColorChan chans)
    {
        int fbfd = open("/dev/fb1", O_RDWR);

        if (fbfd == -1)
        {
            DBG_WRITE("Could not open /dev/fb1");
            panic();
        }

        int screen_datasize = screen_rect->size.w * screen_rect->size.h * chans;
        void * fbp = mmap(0, screen_datasize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
        close(fbfd);

        canvas = new NXCanvas{ NXBitmap{(uint8_t *)fbp, screen_rect, chans} };
    }
};



void validate_args(int argc, char * const argv[]);

bool write_str(int fd, const char * msg);

int PWBase = 0;
int PWLen  = 0;

char table[] = "0123456789" "ABCDEF";

void write_cmd(int i2cfd, U8 * data, int bytes)
{
#ifdef __linux__ 
    if (bytes <= 16)
    {
        int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x00, bytes, data);
        if (res)
            fprintf(stderr, "res = %0d\n", res);
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
                fprintf(stderr, "res = %0d\n", res);
            start += 16;
            num_left -= 16;
        }
    }
#endif
    /*
    for (int i = 0; i < bytes; i++)
    {
        int res = i2c_smbus_write_byte_data(i2cfd, 0x00, data[i]);
        fprintf(stderr, "res = %0d\n", res);
        //write_str(2, "Could not write to /dev/i2c-1\n");
        //exit(1);
    }
    */
}

void write_data(int i2cfd, U8 * data, int bytes)
{
#ifdef __linux__ 
    if (bytes <= 16)
    {
        int res = i2c_smbus_write_i2c_block_data(i2cfd, 0x40, bytes, data);
        if (res)
            fprintf(stderr, "res = %0d\n", res);
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
                fprintf(stderr, "res = %0d\n", res);
            start += 16;
            num_left -= 16;
        }
    }
    //for (int i = 0; i < bytes; i++)
    //{
        //int res = i2c_smbus_write_byte_data(i2cfd, 0x40, data[i]);
        // Usually response is == 0
        //fprintf(stderr, "res = %0d\n", res);
        //write_str(2, "Could not write to /dev/i2c-1\n");
        //exit(1);
    //}
#endif
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

void render_fb(int i2cfd, NXCanvas * canvas)
{
    dbg_render_fb(canvas);
    U8 buff[128];

    // Set the col and row "address"
    unsigned char setFullRange[6] = {0x21,0x00,0x7F,0x22,0x00,0x07};

    // 8 segments (groups of rows)
    for (int seg = 0; seg < 8; seg++)
    {
        setFullRange[4] = seg;
        write_cmd(i2cfd, setFullRange, 6);

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

        write_data(i2cfd, buff, 128);

        //for (int i = 0; i < 128; i++)
        //    write(1, buff[i] ? "X" : ".", 1);
        //write(1, " W\n", 3);
    }
}

int main(int argc, char * const argv[])
{
    //validate_args(argc, argv);

    // Open hardware random number generator
    //
#ifdef __linux__ 
    int i2cfd = open("/dev/i2c-1", O_RDONLY);
    if (i2cfd == -1)
    {
        write_str(2, "Could not open /dev/i2c-1\n");
        exit(1);
    }

    if (ioctl(i2cfd, I2C_SLAVE, 0x3c) == -1)
    {
        write_str(2, "Could not set i2c address\n");
        exit(1);
    }
#else
    int i2cfd;
#endif


    NXRect screen_rect = {0, 0, 128, 64};
    int screen_datasize = screen_rect.size.w * screen_rect.size.h * 1;
    /*
    void * fbp = mmap(0, screen_datasize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (fbp == (void *) -1)
    {
        write_str(2, "Could not mmap some mem\n");
        exit(1);
    }
    */
    void * fbp = calloc(1, screen_datasize);

    NXCanvas screen { NXBitmap{(uint8_t *)fbp, screen_rect, NXColorChan::GREY1} }; 


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

    write_cmd(i2cfd, initSequence, 25);
    //i2c_smbus_write_block_data(i2cfd, 0x00, initSequence+1, 25);
    write_str(2, "Did init writes \n");

    // set the range we want to use (whole display)
    unsigned char setFullRange[6] = {0x21,0x00,0x7F,0x22,0x00,0x07};
    write_cmd(i2cfd, setFullRange,6);
    //write_str(2, "Did write_block\n");

    // send the letter A to the display
    unsigned char letterA[5] = {0x7E,0x12,0x12,0x7E};
    write_data(i2cfd, letterA, 4);

    unsigned char lettera[5] = {0x12, 0x7E, 0x7E, 0x12};
    write_data(i2cfd, lettera, 4);

#ifdef __linux__
    char font_path[] = "/boot/KeyBox/tewi-11.png";
#else
    char font_path[] = "/Users/dru/Downloads/tewi-1.png";
#endif
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
    fprintf(stderr, "font width: %d height: %d chans: %d\n", width, height, chans);


    NXFontAtlas * font = new NXFontAtlas();
    font->atlas = font_bmp;
    font->rect  = { { 0, 0 }, { 192, 104 } };
    font->size  = { 32, 8 };
    font->init();

    screen.fill_rect(&screen_rect, screen.state.bg);
    fprintf(stderr, "did fill_rect\n");
    screen.draw_font(font, NXPoint{0, 3}, "========");
    screen.draw_font(font, NXPoint{0,11}, "Test 123");
    screen.draw_font(font, NXPoint{0,24}, "0x123456789ABCDEF");
    fprintf(stderr, "did draw_font\n");

    render_fb(i2cfd, &screen);
}

void arg_help()
{
    write_str(2, "usage: kb-genrand <base> <length>\n");
    write_str(2, "       base must be either 10 or 16\n");
    write_str(2, "       length must be between 8 and 80\n");
    write_str(2, "\n");
    write_str(2, "   ex: kb-genrand 16 64\n");
    exit(1);
}

void validate_args(int argc, char * const argv[])
{
    if (argc != 3)
        arg_help();

    PWBase = atoi(argv[1]);

    if ((PWBase != 10) && (PWBase != 16)) 
        arg_help();


    PWLen = atoi(argv[2]);

    if ((PWLen < 8) ||(PWLen > 80)) 
        arg_help();
}

bool write_str(int fd, const char * msg)
{
    int len = strlen(msg);
    if (write(fd, msg, len) != len)
        return false;

    return true;
}
