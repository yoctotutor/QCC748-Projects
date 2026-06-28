#include <stdio.h>

#include <string.h>

#include <stdint.h>

#include "qcc74x_mtimer.h"

#include "qcc74x_i2c.h"

#include "qcc74x_adc.h" 

#include "qcc74x_gpio.h"

#include "board.h"



// ================= CONFIG =================

#define RTC_ADDR 0x68

#define OLED_ADDR 0x3C

#define DHT_PIN GPIO_PIN_0 // DHT11 Data Pin

// MQ-135 is on GPIO 19 (ADC Channel 1)

// ==========================================



static struct qcc74x_device_s *i2c0;

static struct qcc74x_device_s *adc;

static struct qcc74x_device_s *gpio; // For DHT11

static uint8_t rtc_reg = 0x00;



// 5x7 Font Table

const uint8_t font5x7[][5] = {

{0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},

{0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},

{0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},

{0x00, 0x41, 0x22, 0x1C, 0x00}, {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08},

{0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},

{0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},

{0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},

{0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},

{0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},

{0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},

{0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},

{0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},

{0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},

{0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},

{0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},

{0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},

{0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},

{0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},

{0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},

{0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00},

{0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04},

{0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78},

{0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F},

{0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E},

{0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00},

{0x7F, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78},

{0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7C, 0x14, 0x14, 0x14, 0x08},

{0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},

{0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},

{0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C},

{0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00},

{0x00, 0x41, 0x36, 0x08, 0x00}, {0x10, 0x08, 0x08, 0x10, 0x08},

};



// ================= DHT11 DRIVER (NEW) =================

static void delay_us(uint32_t us) { qcc74x_mtimer_delay_us(us); }



int dht11_read(uint8_t *temp, uint8_t *hum) {

uint8_t data[5] = {0};

uint32_t timeout;



// 1. Send Start Signal

qcc74x_gpio_init(gpio, DHT_PIN, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);

qcc74x_gpio_reset(gpio, DHT_PIN);

qcc74x_mtimer_delay_ms(20); // Low for >18ms

qcc74x_gpio_set(gpio, DHT_PIN);

delay_us(40); // High for 20-40us



// 2. Switch to Input

qcc74x_gpio_init(gpio, DHT_PIN, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);



// 3. Wait for Sensor Response (Low 80us -> High 80us)

timeout = 300; while (qcc74x_gpio_read(gpio, DHT_PIN) && timeout--) delay_us(1); if(!timeout) return -1;

timeout = 300; while (!qcc74x_gpio_read(gpio, DHT_PIN) && timeout--) delay_us(1); if(!timeout) return -1;

timeout = 300; while (qcc74x_gpio_read(gpio, DHT_PIN) && timeout--) delay_us(1); if(!timeout) return -1;



// 4. Read 40 Bits

for (int i = 0; i < 40; i++) {

timeout = 300; while (!qcc74x_gpio_read(gpio, DHT_PIN) && timeout--) delay_us(1); // Wait for Low start



delay_us(50); // Wait 50us to sample

if (qcc74x_gpio_read(gpio, DHT_PIN)) {

data[i / 8] |= (1 << (7 - (i % 8))); // It's a '1'

}



timeout = 300; while (qcc74x_gpio_read(gpio, DHT_PIN) && timeout--) delay_us(1); // Wait for High end

}



// 5. Verify Checksum

if ((data[0] + data[1] + data[2] + data[3]) != data[4]) return -2;



*hum = data[0];

*temp = data[2];

return 0;

}



// ================= ISOLATED ADC FUNCTION =================

// This function initializes the ADC, reads it, and then kills it

// so it doesn't block the I2C bus for the OLED/RTC.

uint32_t adc_mq135_read_safe(void) {

struct qcc74x_adc_config_s cfg;

struct qcc74x_adc_channel_s chan[] = {

{ .pos_chan = ADC_CHANNEL_1, .neg_chan = ADC_CHANNEL_GND }

};

uint32_t raw_data = 0;

struct qcc74x_adc_result_s result;



// 1. Late Init of ADC Pins

// board_adc_gpio_init();



adc = qcc74x_device_get_by_name("adc");

if (!adc) return 0;



cfg.clk_div = ADC_CLK_DIV_32;

cfg.scan_conv_mode = false;

cfg.continuous_conv_mode = true; // Use Continuous Mode

cfg.differential_mode = false;

cfg.resolution = ADC_RESOLUTION_16B;

cfg.vref = ADC_VREF_3P2V;



qcc74x_adc_init(adc, &cfg);

qcc74x_adc_channel_config(adc, chan, 1);



// 2. Perform Reading

qcc74x_adc_start_conversion(adc);



// Discard first 5 samples (stabilization)

for(int i=0; i<5; i++) {

while(qcc74x_adc_get_count(adc) == 0) qcc74x_mtimer_delay_ms(1);

qcc74x_adc_read_raw(adc);

}



// Keep 6th sample

while(qcc74x_adc_get_count(adc) == 0) qcc74x_mtimer_delay_ms(1);

raw_data = qcc74x_adc_read_raw(adc);



// 3. Stop & Release Resources

qcc74x_adc_stop_conversion(adc);

qcc74x_adc_deinit(adc);



qcc74x_adc_parse_result(adc, &raw_data, &result, 1);

return result.millivolt;

}



// ================= OLED DRIVER (Block Write) =================

// Note: We use Block Write to keep I2C fast and stable

void oled_write_block(uint8_t *data, uint16_t len) {

struct qcc74x_i2c_msg_s msgs[1];

uint8_t buf[129];

buf[0] = 0x40; memcpy(&buf[1], data, len);

msgs[0].addr = OLED_ADDR; msgs[0].flags = I2C_M_WRITE; msgs[0].buffer = buf; msgs[0].length = len + 1;

qcc74x_i2c_transfer(i2c0, msgs, 1);

}



void oled_write_byte(uint8_t dat, uint8_t cmd) {

struct qcc74x_i2c_msg_s msgs[2]; uint8_t msg0_buf[2];

msg0_buf[0] = (cmd == 0) ? 0x00 : 0x40; msg0_buf[1] = dat;

msgs[0].addr = OLED_ADDR; msgs[0].flags = I2C_M_WRITE; msgs[0].buffer = msg0_buf; msgs[0].length = 2;

qcc74x_i2c_transfer(i2c0, msgs, 1);

}



void oled_init(void) {

qcc74x_mtimer_delay_ms(100);

oled_write_byte(0xAE, 0); oled_write_byte(0x20, 0); oled_write_byte(0x02, 0);

oled_write_byte(0xB0, 0); oled_write_byte(0xC8, 0); oled_write_byte(0x00, 0);

oled_write_byte(0x10, 0); oled_write_byte(0x40, 0); oled_write_byte(0x81, 0);

oled_write_byte(0xFF, 0); oled_write_byte(0xA1, 0); oled_write_byte(0xA6, 0);

oled_write_byte(0xA8, 0); oled_write_byte(0x3F, 0); oled_write_byte(0x8D, 0);

oled_write_byte(0x14, 0); oled_write_byte(0xAF, 0);

}



void oled_set_cursor(uint8_t page, uint8_t col) {

col += 2;

oled_write_byte(0xB0 + page, 0);

oled_write_byte(0x00 | (col & 0x0F), 0);

oled_write_byte(0x10 | ((col >> 4) & 0x0F), 0);

}



// [FIX] Use Block Write to save the Bus

void oled_clear_all(void) {

printf("[DBG] Clearing Screen...\r\n");

uint8_t empty[128]; memset(empty, 0, 128);

for (uint8_t page = 0; page < 8; page++) {

oled_set_cursor(page, 0);

oled_write_block(empty, 128);

}

}



void oled_print_char(char c) {

if (c < 32 || c > 126) c = '?';

const uint8_t *bitmap = font5x7[c - 32];

for (uint8_t i = 0; i < 5; i++) oled_write_byte(bitmap[i], 1);

oled_write_byte(0x00, 1);

}



void oled_print(uint8_t page, uint8_t col, char *str) {

oled_set_cursor(page, col);

while (*str) {

oled_print_char(*str++);

}

}



// ================= RTC LOGIC =================

#define PACK(h, l) (uint8_t)((( (h) - '0') << 4) | ((l) - '0'))

#define UN_HI(b) (char)(((b) >> 4) + '0')

#define UN_LO(b) (char)(((b) & 0x0F) + '0')



void packRTC(uint8_t *r) {

r[0] = PACK(__TIME__[6], __TIME__[7]) & 0x7F; r[1] = PACK(__TIME__[3], __TIME__[4]);

r[2] = PACK(__TIME__[0], __TIME__[1]); r[3] = 1;

r[4] = PACK((__DATE__[4] == ' ' ? '0' : __DATE__[4]), __DATE__[5]);

const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

for (int i = 0; i < 12; i++) { if (strncmp(&months[i * 3], __DATE__, 3) == 0) { r[5] = ((i + 1) / 10 << 4) | ((i + 1) % 10); break; } }

r[6] = PACK(__DATE__[9], __DATE__[10]);

}



void unpackRTC(uint8_t *r, char* line1, char* line2) {

const char *m[] = {"???","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

int m_idx = ((r[5] >> 4) * 10) + (r[5] & 0x0F);

sprintf(line1, "%s %c%c 20%c%c", m[m_idx], (r[4]>>4 ? UN_HI(r[4]):' '), UN_LO(r[4]), UN_HI(r[6]), UN_LO(r[6]));

sprintf(line2, "%c%c:%c%c:%c%c", UN_HI(r[2]&0x3F), UN_LO(r[2]), UN_HI(r[1]), UN_LO(r[1]), UN_HI(r[0]&0x7F), UN_LO(r[0]));

}



static void rtc_read(uint8_t *buf) {

struct qcc74x_i2c_msg_s msg[2];

msg[0].addr = RTC_ADDR; msg[0].flags = I2C_M_NOSTOP; msg[0].buffer = &rtc_reg; msg[0].length = 1;

msg[1].addr = RTC_ADDR; msg[1].flags = I2C_M_READ; msg[1].buffer = buf; msg[1].length = 7;

int ret = qcc74x_i2c_transfer(i2c0, msg, 2);

if(ret != 0) printf("[ERR] RTC Fail: %d\r\n", ret);

}



static void rtc_set_default(void) {

uint8_t t[7]; packRTC(t);

struct qcc74x_i2c_msg_s msg[2];

msg[0].addr = RTC_ADDR; msg[0].flags = I2C_M_NOSTOP; msg[0].buffer = &rtc_reg; msg[0].length = 1;

msg[1].addr = RTC_ADDR; msg[1].flags = 0; msg[1].buffer = t; msg[1].length = 7;

qcc74x_i2c_transfer(i2c0, msg, 2);

}



// ================= MAIN =================

int main(void)

{

uint8_t rtc[7];

char line1[20], line2[20];

char aqi_str[20], pure_str[20];

char temp_str[20], hum_str[20];

uint32_t mq_mv = 0, aqi_idx = 0, purity_pct = 0;

uint8_t temp = 0, hum = 0;



board_init();

board_adc_gpio_init();

board_i2c0_gpio_init();



// NOTE: DO NOT call board_adc_gpio_init() here! It breaks I2C.

// We call it inside adc_mq135_read_safe().



i2c0 = qcc74x_device_get_by_name("i2c0");

qcc74x_i2c_init(i2c0, 100000);



gpio = qcc74x_device_get_by_name("gpio"); // Handle for DHT11



printf("Starting System: RTC + OLED + MQ135 + DHT11...\r\n");



oled_init();

oled_clear_all();



rtc_read(rtc);

if (rtc[0] & 0x80) rtc_set_default();



while (1) {

printf("\r\n--- Loop ---\r\n");



// 1. Read RTC

rtc_read(rtc);

unpackRTC(rtc, line1, line2);

printf("RTC: %s %s\r\n", line1, line2);



// 2. Read ADC (Safe Mode)

mq_mv = adc_mq135_read_safe();



if (dht11_read(&temp, &hum) == 0) {

sprintf(temp_str, "Temp: %d C", temp);

sprintf(hum_str, "Hum : %d %%", hum);

printf("DHT: %dC %d%%\r\n", temp, hum);

} else {

sprintf(temp_str, "Temp: Err");

sprintf(hum_str, "Hum : Err");

printf("DHT: Read Error\r\n");

}



// Calc Stats

if(mq_mv > 3300) mq_mv = 3300;

purity_pct = 100 - ((mq_mv * 100) / 3300);

aqi_idx = (mq_mv * 500) / 3300;



sprintf(aqi_str, "AQI: %lu", aqi_idx);

sprintf(pure_str, "Pure: %lu%%", purity_pct);

printf("MQ: %lu mV | %s\r\n", mq_mv, aqi_str);



// 3. Update OLED

oled_print(0, 0, line1);

oled_print(2, 40, line2);

oled_print(4, 0, temp_str); // Temp

oled_print(5, 0, hum_str); // Hum

oled_print(6, 0, aqi_str);

oled_print(7, 0, pure_str);



qcc74x_mtimer_delay_ms(1000);

}

}
