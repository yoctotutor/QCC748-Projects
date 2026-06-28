#include <stdio.h>
#include "qcc74x_mtimer.h"
#include "qcc74x_spi.h"
#include "qcc74x_gpio.h"
#include "qcc74x_uart.h"
#include "board.h"

#define SPI_CS_PIN      GPIO_PIN_12
#define SPI_SCK_PIN     GPIO_PIN_13
#define SPI_MISO_PIN    GPIO_PIN_14
#define SPI_MOSI_PIN    GPIO_PIN_15

static struct qcc74x_device_s *spi0;
static struct qcc74x_device_s *gpio;


void spi_slave_init(void) {
    gpio = qcc74x_device_get_by_name("gpio");
    qcc74x_gpio_init(gpio, SPI_CS_PIN,   GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    qcc74x_gpio_init(gpio, SPI_SCK_PIN,  GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    qcc74x_gpio_init(gpio, SPI_MISO_PIN, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    qcc74x_gpio_init(gpio, SPI_MOSI_PIN, GPIO_FUNC_SPI0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    struct qcc74x_spi_config_s spi_cfg = {
        .freq = 1000000, .role = SPI_ROLE_SLAVE, .mode = SPI_MODE3,
        .data_width = SPI_DATA_WIDTH_8BIT, .bit_order = SPI_BIT_MSB, .byte_order = SPI_BYTE_LSB,
        .tx_fifo_threshold = 0, .rx_fifo_threshold = 0,
    };
    spi0 = qcc74x_device_get_by_name("spi0");
    qcc74x_spi_init(spi0, &spi_cfg);
}

int main(void) {
    board_init();
    spi_slave_init();

    printf("\r\n=== SLAVE READY: HYDERABAD STATION ===\r\n");

    uint8_t rx_byte;
    uint8_t buffer[64];
    int idx = 0;

    while (1) {
        // 1. Wait for Header Byte (0xAA)
        if (qcc74x_spi_poll_exchange(spi0, NULL, &rx_byte, 1) == 0) {

            if (rx_byte == 0xAA) {
                // Header found! Prepare to read the rest of the packet.
                idx = 0;
                memset(buffer, 0, 64);

                // 2. Read bytes while CS is LOW
                // (Master holds CS low during the whole transaction)
                while (qcc74x_gpio_read(gpio, SPI_CS_PIN) == 0) {
                    if (qcc74x_spi_poll_exchange(spi0, NULL, &rx_byte, 1) == 0) {
                        if (idx < 63) {
                            buffer[idx++] = rx_byte;
                        }
                    }
                }

                // 3. Process the Packet
                if (idx > 0) {
                    // Is it the Binary Sensor Packet? (Length usually 7-8 bytes)
                    // Checks if buf[0] is reasonable temperature (0-100) and buf[1] is humidity (0-100)
                    // This heuristic separates binary data from text strings.
                    if (idx == 8 && buffer[0] < 100 && buffer[1] < 100) {
                        uint8_t t   = buffer[0];
                        uint8_t hum = buffer[1];
                        uint16_t aqi_idx = (uint16_t)buffer[2];
                        // Note: Master code sends tx_buf[3] as AQI_IDX, not raw bytes 6/7 anymore.
                        // tx_buf[0]=AA (header, consumed), tx[1]=t, tx[2]=h, tx[3]=aqi

                        printf(">>> SENSOR DATA: T:%d C | H:%d %% | AQI Index: %d\r\n", t, hum, aqi_idx);
                    }
                    // Otherwise, treat it as an OLED String (Text)
                    else {
                        // Ensure null termination
                        buffer[idx] = '\0';
                        printf("OLED TEXT: %s\r\n", buffer);
                    }
                }
            }
        }
    }
}
