#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <Vector.h>
#include "storage.hpp"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// The SSD1351 is connected like this (plus VCC plus GND)
const uint8_t PIN_SCL_SCK = 13;
const uint8_t PIN_SDA_MOSI = 11;
const uint8_t PIN_CS_SS = 10;
const uint8_t PIN_RES_RST = 9;
const uint8_t PIN_DC_RS = 8;
const uint8_t PIN_RESET_RX_BUFF = 2;
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_BLUE = 0x001F;
const uint16_t COLOR_RED = 0xF800;
const uint16_t COLOR_GREEN = 0x07E0;
const uint16_t COLOR_CYAN = 0x07FF;
const uint16_t COLOR_MAGENTA = 0xF81F;
const uint16_t COLOR_YELLOW = 0xFFE0;
const uint16_t COLOR_WHITE = 0xFFFF;

// incoming serial_rx traffic
struct owsd_page new_head;
struct owsd_page top_page = OWSD_PAGE_INIT(new_head);

int b_state = 0;

Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        PIN_CS_SS,
        PIN_DC_RS,
        PIN_RES_RST);

void blink_led()
{
    for (;;)
    {
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
    }
}

struct owsd_page *clear_screen()
{
    digitalWrite(LED_BUILTIN, HIGH);
    oled.fillScreen(COLOR_BLACK);
    free(&top_page);
    top_page = OWSD_PAGE_INIT(new_head);
    b_state = 0;
}

void reset_cursor()
{
    oled.setCursor(0, 0);
}

void display(struct owsd_page *page)
{
    reset_cursor();
    while (page != NULL)
    {

        if (page->data != NULL)
            Serial.println(page->data);
        oled.print(page->data);

        page = page->next;
    }
}

void setup()
{
    pinMode(PIN_RESET_RX_BUFF, INPUT);
    Serial.begin(9600);

    delay(250);
    oled.begin();
    oled.setFont();
    oled.fillScreen(COLOR_BLACK);
    oled.setTextColor(COLOR_WHITE);
    oled.setTextSize(1);
}

int count = 0;
void loop()
{
    // // if we try to reset the display
    // b_state = digitalRead(PIN_RESET_RX_BUFF);
    // if (b_state == 1)
    //     clear_screen();

    if (Serial.available() > 0)
    {
        char rx_byte = (char)Serial.read();
        struct owsd_page *new_page = alloc_owsd_page(rx_byte);
        if (new_page == NULL)
            oled.println("Out of Memory");
        list_add(&top_page, new_page);
    }
    if (count == 0)
    {
        struct owsd_page *new_page = alloc_owsd_page('a');
        if (new_page == NULL) {
            oled.setTextColor(COLOR_RED);
            oled.println("Out of Memory");
        }
        list_add(&top_page, new_page);
        count++;
    }

    display(&top_page);
    delay(1);
}