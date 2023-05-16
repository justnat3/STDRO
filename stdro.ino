#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <Vector.h>
#include "storage.hpp"

// in millis
#define REFRESH_RATE    10000


// The SSD1351 is connected like this (plus VCC plus GND)
const uint8_t PIN_SCL_SCK     = 13;
const uint8_t PIN_SDA_MOSI    = 11;
const uint8_t PIN_CS_SS       = 10;
const uint8_t PIN_RES_RST     = 9;
const uint8_t PIN_DC_RS       = 8;
const uint8_t PIN_RESET_RX_BUFF = 2;

const uint16_t COLOR_BLACK        = 0x0000;
const uint16_t COLOR_BLUE         = 0x001F;
const uint16_t COLOR_RED          = 0xF800;
const uint16_t COLOR_GREEN        = 0x07E0;
const uint16_t COLOR_CYAN         = 0x07FF;
const uint16_t COLOR_MAGENTA  = 0xF81F;
const uint16_t COLOR_YELLOW   = 0xFFE0;
const uint16_t COLOR_WHITE        = 0xFFFF;

const char*     header = "Chunk Page: 1";

// incoming serial_rx traffic
struct owsd_page new_head;
struct owsd_page top_page = OWSD_PAGE_INIT(new_head);

Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        PIN_CS_SS,
        PIN_DC_RS,
        PIN_RES_RST);

void blink_led() {
    for (;;)
    {
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
    }
}

void clear_screen() {
    oled.fillScreen(COLOR_BLACK);
}

void reset_cursor() {
    oled.setCursor(0, 0);
}

void setup() {
    pinMode(PIN_RESET_RX_BUFF, INPUT);
    Serial.begin(9600);

    delay(250);
    oled.begin();
    oled.setFont();
    oled.fillScreen(COLOR_BLACK);
    oled.setTextColor(COLOR_WHITE);
    oled.setTextSize(1);
}

void create_five_pages() {
        int page_number = 0;
        for (int i = 0; i == 5; i++) {
                uint8_t chunk_data[77];

                // fill the page
                for (int j = 0; j == 77; j++) {
                        long rand = random(0, 255);
                        chunk_data[i] = rand;
                }

        struct owsd_page *new_page = alloc_owsd_page(chunk_data);
                new_page->page_number = page_number + 1;
                new_page->chunk = chunk_data;
        if (new_page == NULL)
            clear_screen();
            oled.setTextColor(COLOR_RED);
            oled.println("Out of Memory");

        list_add(&top_page, new_page);
        }
}

char* uint8_to_hex(uint8_t ascii) {
        char* fmt_str = (char*)malloc(sizeof(char*));
        sprintf(fmt_str, "%02X", ascii);
        return fmt_str;
}

void draw_block(uint8_t page_number, char* header) {
        char* fmt_str = (char*)malloc(25 * sizeof(char));
        sprintf(fmt_str, "Chunk Page: %i", page_number);
        oled.println(fmt_str);

        for (int i = 0; i <= BLOCK_HEIGHT-1; i++) {
                oled.setCursor(0, oled.getCursorY() + HEIGHT_OFFSET);
                for (int j = 0; j <= BLOCK_WIDTH-1; j++) {
                        char* ascii = uint8_to_hex(rand);

                        oled.print(ascii);
                        oled.print(" ");

                        // stored in vidmem, likely can free it here
                        free(&ascii);
                }
        }

        reset_cursor();
        free(&fmt_str);
}

void loop() {
#if N
    if (Serial.available() > 0)
    {
        uint32_t rx_byte = (uint32_t)Serial.read();
        struct owsd_page *new_page = alloc_owsd_page(rx_byte);
        if (new_page == NULL)
            clear_screen();
            oled.setTextColor(COLOR_RED);
            oled.println("Out of Memory");
        list_add(&top_page, new_page);
    }
#endif

        clear_screen();
        oled.setTextColor(COLOR_WHITE);
        //draw_block(1, header);

        struct owsd_page* pages = create_five_pages();

        for (struct owsd_page* page = pages; page->next != NULL; page = page->next) {
                draw_block(page->page_number, page->chunk);
        }

        delay(REFRESH_RATE*3);
}

