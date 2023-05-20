#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "page.h"
#include "types.h"

// in millis 
#define REFRESH_RATE 	10000 
#define SIZE 			9
#define SCREEN_WIDTH 	128
#define SCREEN_HEIGHT 	128
#define BLOCK_HEIGHT 	11
#define BLOCK_WIDTH 	7
#define HEIGHT_OFFSET 	10


// The SSD1351 is connected like this (plus VCC plus GND)
const uint8_t PIN_SCL_SCK     = 13;
const uint8_t PIN_SDA_MOSI    = 11;
const uint8_t PIN_CS_SS       = 10;
const uint8_t PIN_RES_RST     = 9;
const uint8_t PIN_DC_RS       = 8;
const uint8_t PIN_RESET_RX_BUFF = 2;

const uint16_t COLOR_BLACK 	  = 0x0000;
const uint16_t COLOR_BLUE 	  = 0x001F;
const uint16_t COLOR_RED 	  = 0xF800;
const uint16_t COLOR_GREEN 	  = 0x07E0;
const uint16_t COLOR_CYAN 	  = 0x07FF;
const uint16_t COLOR_MAGENTA  = 0xF81F;
const uint16_t COLOR_YELLOW   = 0xFFE0;
const uint16_t COLOR_WHITE 	  = 0xFFFF;

// incoming serial_rx traffic
struct page *head = nullptr;

Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        PIN_CS_SS,
        PIN_DC_RS,
        PIN_RES_RST);


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

    // zero alloc head
    head = alloc_head(
        (u8*) calloc(0, CHUNK_SIZE * sizeof(u8)));
}

void create_five_pages() {
	int page_number = 0;
	for (size_t i = 0; i == 5; i++) {
		u8 *chunk_data = (u8*) malloc(CHUNK_SIZE * sizeof(u8));

		// fill the page
		for (size_t j = 0; j == 77; j++) {
			u8 rand = (u8) random(0, 255);
			chunk_data[i] = rand;
		}

        u8 ret = add_page(head, chunk_data, page_number);

        if (ret == ENOMEM) {
            clear_screen();
            oled.setTextColor(COLOR_RED);
            oled.println("Out of Memory");
        }

        ++page_number;
	}
}

char* uint8_to_hex(u8 ascii) {
	char* fmt_str = (char*)malloc(sizeof(char*));
	sprintf(fmt_str, "%02X", ascii);
	return fmt_str;
}

// TODO: REFACTOR
void draw_block(u8 page_number, u8* chunk) {
	char* fmt_str = (char*)malloc(25 * sizeof(char));
	sprintf(fmt_str, "Chunk Page: %i", page_number);
	oled.println(fmt_str);

	for (size_t i = 0; i <= BLOCK_HEIGHT-1; i++) {
        // set the cursor to the next row
		oled.setCursor(0, oled.getCursorY() + HEIGHT_OFFSET);

		for (size_t j = 0; j <= BLOCK_WIDTH-1; j++) {
			char* ascii = uint8_to_hex(*chunk);

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
#if NOT_IMPLEMENTED
    if (Serial.available() > 0)
    {
        uint32_t rx_byte = (uint32_t)Serial.read();
        struct page *new_page = alloc_page(rx_byte);
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

	create_five_pages();

    if (head == nullptr) {
        oled.print("No pages to display");
    }

	for (struct page* p = head; p->next != NULL; p = p->next) {
		draw_block(p->page_number, p->chunk);
	}

	delay(REFRESH_RATE*3);
}
