//
// Created by acite on 7/15/24.
//

#include "st7735.h"

#include <cstring>

void st7735::swap_byte(uint16_t *x) {
    auto *t = (uint8_t*)x;
    uint8_t tx = t[0];
    t[0] = t[1];
    t[1] = tx;
}

st7735::st7735(gpio_num_t dc, gpio_num_t rst) {
    this->dc = dc;
    this->rst = rst;

    gpio_config_t ioConfig = {
            .pin_bit_mask = (1ull << dc)|(1ull << rst),
            .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&ioConfig);

    // SPI

    spi_bus_config_t buscfg={    //总线配置结构体
            .mosi_io_num = 13,    //gpio13->mosi
            .miso_io_num = 12,
            .sclk_io_num = 14,    //gpio14-> sclk
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4094,
            .flags = SPICOMMON_BUSFLAG_MASTER
    };
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t interface_config = { 0 };
    interface_config.address_bits = 0;
    interface_config.command_bits = 0;
    interface_config.clock_speed_hz = 60 * 1000 * 1000;
    interface_config.mode = 0;
    interface_config.spics_io_num = 15;
    interface_config.duty_cycle_pos = 0;
    interface_config.queue_size = 7;
    interface_config.flags = SPI_DEVICE_HALFDUPLEX;

    spi_bus_add_device(SPI2_HOST, &interface_config, &spi2_handle);

    frame_buffer = new uint16_t[tft_width * tft_height];
}

void st7735::write_command(uint8_t cmd) {
    gpio_set_level(dc, 0);

    spi_transaction_t t; //定义数据结构体
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(spi2_handle, &t);

}

void st7735::write_data(uint8_t data) {
    gpio_set_level(dc, 1);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    spi_device_polling_transmit(spi2_handle, &t);
}

void st7735::write_block(uint8_t *data, uint32_t cc) {
    spi_transaction_t t; //定义数据结构体
    memset(&t, 0, sizeof(t));
    t.length = 8 * 1024;

    for(int i=0;i< cc / 1024;i++)
    {
        t.tx_buffer = data + i * 1024;
        spi_device_polling_transmit(spi2_handle, &t);
    }
}

void st7735::write_r_cmd_1() {
    write_command(ST7735_SWRESET);
    esp_rom_delay_us(150 * 1000);
    write_command(ST7735_SLPOUT);
    esp_rom_delay_us(250 * 1000);
    esp_rom_delay_us(250 * 1000);
    write_command(ST7735_FRMCTR1);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR2);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR3);
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_command(ST7735_INVCTR);
    write_data(0x07);
    write_command(ST7735_PWCTR1);
    write_data(0xA2);
    write_data(0x02);
    write_data(0x84);
    write_command(ST7735_PWCTR2);
    write_data(0xC5);
    write_command(ST7735_PWCTR3);
    write_data(0x0A);
    write_data(0x00);
    write_command(ST7735_PWCTR4);
    write_data(0x8A);
    write_data(0x2A);
    write_command(ST7735_PWCTR5);
    write_data(0x8A);
    write_data(0xEE);
    write_command(ST7735_VMCTR1);
    write_data(0x0E);
    write_command(ST7735_INVOFF);
    write_command(ST7735_MADCTL);
    write_data(0xC8);
    write_command(ST7735_COLMOD);
    write_data(0x05);
}

void st7735::write_r_cmd_2() {
    write_command(ST7735_CASET);
    write_data(0x00); write_data(0x00);
    write_data(0x00); write_data(0x7F);
    write_command(ST7735_RASET);
    write_data(0x00); write_data(0x00);
    write_data(0x00); write_data(0x9F);
}

void st7735::write_r_cmd_3() {
    write_command(ST7735_GMCTRP1);
    write_data(0x02); write_data(0x1C); write_data(0x07); write_data(0x12);
    write_data(0x37); write_data(0x32); write_data(0x29); write_data(0x2D);
    write_data(0x29); write_data(0x25); write_data(0x2B); write_data(0x39);
    write_data(0x00); write_data(0x01); write_data(0x03); write_data(0x10);
    write_command(ST7735_GMCTRN1);
    write_data(0x03); write_data(0x1D); write_data(0x07); write_data(0x06);
    write_data(0x2E); write_data(0x2C); write_data(0x29); write_data(0x2D);
    write_data(0x2E); write_data(0x2E); write_data(0x37); write_data(0x3F);
    write_data(0x00); write_data(0x00); write_data(0x02); write_data(0x10);
    write_command(ST7735_NORON);
    esp_rom_delay_us(10 * 1000);
    write_command(ST7735_DISPON);
    esp_rom_delay_us(100 * 1000);
}

void st7735::setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_command(ST7735_CASET);
    write_data(0);
    write_data(x0 + _xstart);
    write_data(0);
    write_data(x1 + _xstart);
    write_command(ST7735_RASET);
    write_data(0);
    write_data(y0 + _ystart);
    write_data(0);
    write_data(y1 + _ystart);
    write_command(ST7735_RAMWR); // Write to RAM
}

void st7735::initBlackTab() {
    reset();
    gpio_set_level(dc, 0);
    write_r_cmd_1();  // Write cmd list 1
    write_r_cmd_2();  // Write cmd list 2
    write_r_cmd_3();  // Write cmd list 3

    write_command(ST7735_MADCTL);
    write_data(0xC0);
}

void st7735::reset() {
    gpio_set_level(rst, 1);
    esp_rom_delay_us(10 * 1000);
    gpio_set_level(rst, 0);
    esp_rom_delay_us(10 * 1000);
    gpio_set_level(rst, 1);
    esp_rom_delay_us(10 * 1000);
}

void st7735::setRotation(uint8_t m) {
    // m can be 0-3
    uint8_t madctl = 0;
    _rotation = m % 4;

    switch (_rotation) {
        case 0:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            break;
        case 1:
            madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
            _ystart = _colstart;
            _xstart = _rowstart;
            break;
        case 2:
            madctl = ST7735_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            break;
        case 3:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
            _ystart = _colstart;
            _xstart = _rowstart;
            break;
    }
    write_command(ST7735_MADCTL);
    write_data(madctl);
}

void st7735::fillRectangle(uint16_t color, uint8_t w, uint8_t h, uint8_t x, uint8_t y) {
    for (uint16_t ly = y ; ly - y < h ; ly ++)
        for (uint16_t lx = x ; lx - x < w ; lx ++)
        {
            frame_buffer[ly * tft_width + lx] = color;
            swap_byte(&frame_buffer[ly * tft_width + lx]);
        }
}

void st7735::show() {
    setWindow(0, 0, tft_width - 1, tft_height - 1);
    gpio_set_level(dc, 1);
    write_block((uint8_t*)frame_buffer, tft_width * tft_height * 2);
}

void st7735::clear(uint16_t color) {
    fillRectangle(color, tft_width, tft_height, 0, 0);
}

void st7735::drawFastHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
    swap_byte(&color);
    for(uint32_t lx=x; lx - x < w;lx++)
    {
        frame_buffer[y * tft_width + lx] = color;
    }
}

void st7735::drawFastVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
    swap_byte(&color);
    for(uint32_t ly=y; ly - y < h;ly++)
    {
        frame_buffer[ly * tft_width + x] = color;
    }
}

void st7735::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    int16_t i;
    for (i = x; i < x + w; i++) {
        drawFastVLine(i, y, h, color);
    }
}

void st7735::drawPixel(uint8_t x, uint8_t y, uint16_t color) {
    swap_byte(&color);
    frame_buffer[y * tft_width + x] = color;
}

void st7735::drawChar(uint8_t x, uint8_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t size) {
    int8_t i, j;
    if((x >= tft_width) || (y >= tft_height))
        return;
    if(size < 1) size = 1;
    if((c < ' ') || (c > '~'))
        c = '?';
    for(i=0; i<5; i++ ) {
        uint8_t line;
        line = Font[(c - LCD_ASCII_OFFSET)*5 + i];
        for(j=0; j<7; j++, line >>= 1) {
            if(line & 0x01) {
                if(size == 1) drawPixel(x+i, y+j, color);
                else          fillRect(x+(i*size), y+(j*size), size, size, color);
            }
            else if(bg != color) {
                if(size == 1) drawPixel(x+i, y+j, bg);
                else          fillRect(x+i*size, y+j*size, size, size, bg);
            }
        }
    }
}

void st7735::drawText(uint8_t x, uint8_t y, const char *_text, uint16_t color, uint16_t bg, uint8_t size) {
    uint8_t cursor_x, cursor_y;
    uint16_t textsize, i;
    cursor_x = x, cursor_y = y;
    textsize = strlen(_text);
    for(i = 0; i < textsize; i++){
        if((cursor_x + size * 5) > tft_width) {
            cursor_x = 0;
            cursor_y = cursor_y + size * 7 + 3 ;
            if(cursor_y > tft_height) cursor_y = tft_height;
            if(_text[i] == LCD_ASCII_OFFSET) {
                continue;
            }
        }
        drawChar(cursor_x, cursor_y, _text[i], color, bg, size);
        cursor_x = cursor_x + size * 6;
        if(cursor_x > tft_width) {
            cursor_x = tft_width;
        }
    }
}

void st7735::drawLine(int x0, int y0, int x1, int y1, uint16_t col) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    while (1) {
        drawPixel(x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void st7735::drawRect(int x, int y, int w, int h, uint16_t col) {
    drawLine(x, y, x + w - 1, y, col);
    drawLine(x, y, x, y + h - 1, col);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, col);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, col);
}

void st7735::drawCircle(int xc, int yc, int r, uint16_t col) {
    int x = 0, y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        drawPixel(xc + x, yc + y, col);
        drawPixel(xc - x, yc + y, col);
        drawPixel(xc + x, yc - y, col);
        drawPixel(xc - x, yc - y, col);
        drawPixel(xc + y, yc + x, col);
        drawPixel(xc - y, yc + x, col);
        drawPixel(xc + y, yc - x, col);
        drawPixel(xc - y, yc - x, col);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void st7735::fillCircle(int xc, int yc, int r, uint16_t col) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                drawPixel(xc + x, yc + y, col);
            }
        }
    }
}


