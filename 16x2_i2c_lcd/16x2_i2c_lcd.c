#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qcc74x_mtimer.h"
#include "qcc74x_i2c.h"
#include "board.h"

void packRTC(uint8_t* r);
void unpackRTC(uint8_t* r,char* lin1,char* line2);

/* ================= CONFIG ================= */

#define RTC_ADDR        0x68
#define LCD_ADDR        0x27

/* ================= I2C HANDLE ================= */

static struct qcc74x_device_s *i2c0;
static uint8_t rtc_reg = 0x00;

/* ================= BCD HELPERS ================= */

/*static uint8_t dec_to_bcd(uint8_t v)
{
    return ((v / 10) << 4) | (v % 10);
}

static uint8_t bcd_to_dec(uint8_t v)
{
    return ((v >> 4) * 10) + (v & 0x0F);
}*/

// Minimal Helpers: b = bcd, c = char
#define PACK(h, l) (uint8_t)((( (h) - '0') << 4) | ((l) - '0'))
#define UN_HI(b)   (char)(((b) >> 4) + '0')
#define UN_LO(b)   (char)(((b) & 0x0F) + '0')

/* ================= LCD DRIVER (PCF8574) ================= */

#define LCD_BL  0x08
#define LCD_EN  0x04
#define LCD_RS  0x01

static void lcd_write(uint8_t data)
{
    struct qcc74x_i2c_msg_s msg;
    msg.addr = LCD_ADDR;
    msg.flags = 0;
    msg.buffer = &data;
    msg.length = 1;
    qcc74x_i2c_transfer(i2c0, &msg, 1);
}

static void lcd_pulse(uint8_t data)
{
    lcd_write(data | LCD_EN);
    qcc74x_mtimer_delay_us(50);
    lcd_write(data & ~LCD_EN);
}

static void lcd_send4(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | LCD_BL;
    if (rs) data |= LCD_RS;
    lcd_write(data);
    lcd_pulse(data);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send4(cmd & 0xF0, 0);
    lcd_send4(cmd << 4, 0);
    qcc74x_mtimer_delay_ms(2);
}

static void lcd_data(uint8_t d)
{
    lcd_send4(d & 0xF0, 1);
    lcd_send4(d << 4, 1);
}

static void lcd_init(void)
{
    qcc74x_mtimer_delay_ms(50);

    lcd_send4(0x30, 0);
    qcc74x_mtimer_delay_ms(5);
    lcd_send4(0x30, 0);
    qcc74x_mtimer_delay_us(200);
    lcd_send4(0x30, 0);
    qcc74x_mtimer_delay_ms(5);
    lcd_send4(0x20, 0);

    lcd_cmd(0x28);  // 4-bit, 2-line
    lcd_cmd(0x0C);  // display ON
    lcd_cmd(0x06);  // entry mode
    lcd_cmd(0x01);  // clear
}

static void lcd_cursor(uint8_t row, uint8_t col)
{
    lcd_cmd((row ? 0xC0 : 0x80) + col);
}

static void lcd_print(const char *s)
{
    while (*s) lcd_data(*s++);
}

/* ================= RTC ================= */

static void rtc_read(uint8_t *buf)
{
    struct qcc74x_i2c_msg_s msg[2];

    msg[0].addr = RTC_ADDR;
    msg[0].flags = I2C_M_NOSTOP;
    msg[0].buffer = &rtc_reg;
    msg[0].length = 1;

    msg[1].addr = RTC_ADDR;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = 7;

    qcc74x_i2c_transfer(i2c0, msg, 2);
}

static void rtc_set_default(void)
{
    uint8_t t[7];

    packRTC(t);

    struct qcc74x_i2c_msg_s msg[2];

    msg[0].addr = RTC_ADDR;
    msg[0].flags = I2C_M_NOSTOP;
    msg[0].buffer = &rtc_reg;
    msg[0].length = 1;

    msg[1].addr = RTC_ADDR;
    msg[1].flags = 0;
    msg[1].buffer = t;
    msg[1].length = 7;

    qcc74x_i2c_transfer(i2c0, msg, 2);
}




/* ================= MAIN ================= */

int main(void)
{
    uint8_t rtc[7];
    char line1[17], line2[17];

    board_init();
    qcc74x_mtimer_delay_ms(300);

    board_i2c0_gpio_init();
    i2c0 = qcc74x_device_get_by_name("i2c0");
    qcc74x_i2c_init(i2c0, 100000);

    lcd_init();

    rtc_read(rtc);
    if (rtc[0] & 0x80) {
        rtc_set_default();
    }

    while (1) {
        rtc_read(rtc);

        unpackRTC(rtc,line1,line2);

        lcd_cmd(0x01);
        lcd_cursor(0, 0);
        lcd_print(line1);
        lcd_cursor(1, 0);
        lcd_print(line2);

        printf("%s  %s\r\n", line1, line2);

        qcc74x_mtimer_delay_ms(1000);
    }
}





// 1. Pack Macros into DS1307 Registers (7 Bytes)
void packRTC(uint8_t *r)
{
    // __TIME__ = "hh:mm:ss"
    r[0] = PACK(__TIME__[6], __TIME__[7]) & 0x7F;  // seconds
    r[1] = PACK(__TIME__[3], __TIME__[4]);         // minutes
    r[2] = PACK(__TIME__[0], __TIME__[1]);         // hours (24h)
    r[3] = 1;  // weekday (can be fixed or calculated)

    // __DATE__ = "Mmm dd yyyy"
    r[4] = PACK((__DATE__[4] == ' ' ? '0' : __DATE__[4]), __DATE__[5]); // day

    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    for (int i = 0; i < 12; i++) {
        if (strncmp(&months[i * 3], __DATE__, 3) == 0) {
            uint8_t m = i + 1;
            r[5] = ((m / 10) << 4) | (m % 10);
            break;
        }
    }

    r[6] = PACK(__DATE__[9], __DATE__[10]); // year (yy)
}


// 2. Unpack DS1307 Registers to Strings
void unpackRTC(uint8_t *r,char* line1,char* line2) {
    const char *m[] = {"???","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int m_idx = ((r[5] >> 4) * 10) + (r[5] & 0x0F);

    // BCD -> "Mmm dd 20yy"
    sprintf(line1, "%s %c%c 20%c%c", m[m_idx], (r[4]>>4 ? UN_HI(r[4]):' '), UN_LO(r[4]), UN_HI(r[6]), UN_LO(r[6]));
    // BCD -> "hh:mm:ss"
    sprintf(line2, "%c%c:%c%c:%c%c", UN_HI(r[2]&0x3F), UN_LO(r[2]), UN_HI(r[1]), UN_LO(r[1]), UN_HI(r[0]&0x7F), UN_LO(r[0]));
}

