#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "types.h"

// in millis
#define REFRESH_RATE 10000
#define SIZE 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define BLOCK_HEIGHT 11
#define BLOCK_WIDTH 7
#define HEIGHT_OFFSET 10
#define NULL nullptr
#define ENOMEM -1
#define CHUNK_SIZE BLOCK_HEIGHT *BLOCK_WIDTH
#define NO_HEAD -2
#define SECOND 1000

// The SSD1351 is connected like this (plus VCC plus GND)
const u8 PIN_SCL_SCK = 13;
const u8 PIN_SDA_MOSI = 11;
const u8 PIN_CS_SS = 10;
const u8 PIN_RES_RST = 9;
const u8 PIN_DC_RS = 8;
const u8 PIN_RESET_RX_BUFF = 2;

const u16 COLOR_BLACK = 0x0000;
const u16 COLOR_BLUE = 0x001F;
const u16 COLOR_RED = 0xF800;
const u16 COLOR_GREEN = 0x07E0;
const u16 COLOR_CYAN = 0x07FF;
const u16 COLOR_MAGENTA = 0xF81F;
const u16 COLOR_YELLOW = 0xFFE0;
const u16 COLOR_WHITE = 0xFFFF;

/*
 * this is a simple linked list, the way I am thinking is that I want to load a "chunk"
 * into memory for the lcd, when the user selects the next chunk we need to look at the next* page
 * then when we reach the end of the pages we should rollover to the first page. head*
 *
 *             If there are 2 pages
 * ----------------------------------------------------- *
 * tail<-head->next             head<-tail->next(head)
 *        |             where           |
 * tail<-prev                   head<-prev
 */

struct page
{
    int *chunk;
    u8 page_number;
    struct page *next, *prev;
};

struct page *head = NULL;
struct page *alloc_head(int *chunk)
{

    // all new pages are links
    struct page *link = (struct page *)malloc(sizeof(struct page));

    link->next = NULL;
    link->prev = NULL;
    link->chunk = chunk; // zero allocated chunk
    link->page_number = 0;

    return link;
}

Adafruit_SSD1351 oled =
    Adafruit_SSD1351(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &SPI,
        PIN_CS_SS,
        PIN_DC_RS,
        PIN_RES_RST);

// this is just linked list append
i8 add_page(struct page *head, int *chunk, u8 page_number)
{
    struct page *tail = head->prev;
    struct page *link = (struct page *)malloc(sizeof(struct page));

    link->chunk = chunk;
    link->page_number = page_number;

    if (link == NULL)
        return ENOMEM;
    if (head == NULL)
        return NO_HEAD;

    // at the start, we only have head so the next node become the tail
    if (head->prev == NULL || head->next == NULL)
    {
        head->next = link;
        head->prev = link;
        return 0;
    }

    // become middle node, making link the last
    tail->next = link;
    // head points to tail because its the last node
    head->prev = link;
    // link is now the tail so it points to the head
    link->next = head;
    // links previous node is the now middle node tail
    link->prev = tail;

    return 0;
}

void reset_cursor()
{
    oled.setCursor(0, 0);
}

void free_pages(struct page *head)
{
    struct page *lk;
    lk = head;
    head->prev->next = NULL; // last node next should be NULL
    while (lk->next != NULL)
    {
        lk = lk->next;
        free(lk->prev);
    }
    free(head);
    head = NULL;
}

void init_head()
{
    int chunk[CHUNK_SIZE] = {0};
    // zero alloc head
    head = alloc_head(chunk);
}

void clear_screen()
{
    reset_cursor();
    oled.fillScreen(COLOR_BLACK);
    oled.setTextColor(COLOR_WHITE);
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

    int chunk[CHUNK_SIZE] = {0};
    // zero alloc head
    head = alloc_head(chunk);
}

void create_five_pages()
{
    for (size_t i = 0; i < 5; i++)
    {
        clear_screen();
        int chunk_data[CHUNK_SIZE] = {0};

        // fill the page
        for (size_t j = 0; j < CHUNK_SIZE; j++)
        {
            chunk_data[j] = (rand() % 254);
        }

        u8 ret = add_page(head, chunk_data, (u8)i + 1);

        if (ret == ENOMEM)
        {
            clear_screen();
            oled.setTextColor(COLOR_RED);
            oled.println("Out of Memory");
        }
    }
}

char **chunk_to_ascii(int *ascii)
{
    char **str = (char **)malloc(CHUNK_SIZE * sizeof(char *));

    for (size_t i = 0; i < CHUNK_SIZE; i++)
    {
        int size = snprintf(NULL, 0, "%02X", ascii[i]);
        char *fmt_str = (char *)malloc(size + 1);
        sprintf(fmt_str, "%02X", ascii[i]);
        str[i] = fmt_str;
    }

    return str;
}

// TODO: REFACTOR
void draw_block(u8 page_number, int *chunk)
{
    // render the header
    char *header = (char *)malloc(14 * sizeof(char));
    sprintf(header, "Chunk Page: %i", page_number);
    char **ascii = chunk_to_ascii(chunk);

    // render the block
    oled.println(header);
    for (size_t i = 0; i < BLOCK_HEIGHT; i++)
    {
        // set the cursor to the next row
        oled.setCursor(0, oled.getCursorY() + HEIGHT_OFFSET);

        for (size_t j = 0; j < BLOCK_WIDTH; j++)
        {
            oled.print(ascii[j]);
            oled.print(" ");
        }
    }

    free(header);
}

void loop()
{
#if NOT_IMPLEMENTED
    if (Serial.available() > 0)
    {
        int rx_byte = (uint32_t)Serial.read();
    }
#endif

    clear_screen();
    oled.setTextColor(COLOR_WHITE);

    create_five_pages();
    delay(1000);

    struct page *lk = (struct page *)malloc(sizeof(struct page));

    // skip first page
    lk = head->next;

    if (lk->next == NULL)
    {
        oled.setTextColor(COLOR_WHITE);
        oled.print("no pages to display");
    }

    while (1)
    {
        clear_screen();
        draw_block(lk->page_number, lk->chunk);
        lk = lk->next;
        delay(2 * SECOND);
    }

    delay(REFRESH_RATE * 3);
}
