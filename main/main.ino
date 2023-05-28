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
#define CHUNK_SIZE 77
#define NO_HEAD -2
#define SECOND 1000
#define FULL -1
#define NOT_FULL 1

int no_page = 0;
static const u8 PIN_SCL_SCK = 13;
static const u8 PIN_SDA_MOSI = 11;
static const u8 PIN_CS_SS = 10;
static const u8 PIN_RES_RST = 9;
static const u8 PIN_DC_RS = 8;
static const u8 PIN_RESET_RX_BUFF = 2;

static const u16 COLOR_BLACK = 0x0000;
static const u16 COLOR_BLUE = 0x001F;
static const u16 COLOR_RED = 0xF800;
static const u16 COLOR_GREEN = 0x07E0;
static const u16 COLOR_CYAN = 0x07FF;
static const u16 COLOR_MAGENTA = 0xF81F;
static const u16 COLOR_YELLOW = 0xFFE0;
static const u16 COLOR_WHITE = 0xFFFF;

static Adafruit_SSD1351 oled =
  Adafruit_SSD1351(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    PIN_CS_SS,
    PIN_DC_RS,
    PIN_RES_RST);

typedef struct Page {
  int chunk[CHUNK_SIZE];
  short page_number;
  short write_ptr;
  struct Page *next;
  struct Page *prev;
} Page;

Page *head;
Page *current_lk;

// this is just linked list append
void add_page(Page *link) {
  if (head == NULL)
    halt_msg("no_head");

  link->page_number = head->prev->page_number += 1;
  link->write_ptr = 0;

  clear_screen();
  oled.println("allocated a page! ");
  oled.print("Page Number: ");
  oled.print(link->page_number);
  delay(SECOND);

  // at the start, we only have head so the next node become the tail
  if (head->prev == NULL) {
    head->next = link;
    head->prev = link;
  }

  Page *tail = head->prev;
  if (tail == NULL)
    halt_msg("no tail");

  // become middle node, making link the last
  tail->next = link;
  // head points to tail because its the last node
  head->prev = link;
  // link is now the tail so it points to the head
  link->next = head;
  // links previous node is the now middle node tail
  link->prev = tail;
}

void reset_cursor() {
  oled.setCursor(0, 0);
}

void free_pages(Page *head) {
  Page *lk;
  lk = head;
  head->prev->next = NULL;  // last node next should be NULL
  while (lk->next != NULL) {
    lk = lk->next;
    free(lk->prev);
  }
  free(head);
  head = NULL;
}


void clear_screen() {
  reset_cursor();
  oled.fillScreen(COLOR_BLACK);
  oled.setTextColor(COLOR_WHITE);
}

char **chunk_to_ascii(int *ascii) {
  char **str;

  for (size_t i = 0; i < CHUNK_SIZE; i++) {
    int size = snprintf(NULL, 0, "%02X", ascii[i]);

    char *fmt_str = (char *)malloc(size + 1);
    if (fmt_str == NULL)
      oom("2chunk_to_ascii");

    sprintf(fmt_str, "%02X", ascii[i]);
    str[i] = fmt_str;
  }

  return str;
}

// TODO: REFACTOR
void draw_block(Page *link) {
  // render the header
  char *header = (char *)malloc(14 * sizeof(char));
  if (header == NULL)
    oom("draw_block");

  sprintf(header, "Chunk Page: %i", link->page_number);
  char **ascii = chunk_to_ascii(link->chunk);

  // render the block
  oled.println(header);
  for (size_t i = 0; i < BLOCK_HEIGHT; i++) {
    // set the cursor to the next row
    oled.setCursor(0, oled.getCursorY() + HEIGHT_OFFSET);

    for (size_t j = 0; j < BLOCK_WIDTH; j++) {
      oled.print(ascii[j]);
      oled.print(" ");
    }
  }

  // cleanup
  for (size_t a = 0; a < CHUNK_SIZE; a++)
    free(ascii[a]);
  free(ascii);
  free(header);
}

i8 chunk_full(Page *page) {
  // this is not full because we write these positions when the chunk is allocated
  if (page->chunk[CHUNK_SIZE - 2] == 0xFF && page->chunk[CHUNK_SIZE - 1] == 0xAA)
    return NOT_FULL;

  return FULL;
}

// we alloc an empty chunk with a magic marker at the end of the check
// this is so we can check if the chunk is full or not
void alloc_empty_chunk(Page *page) {
  clear_screen();
  oled.print("Allocated Empty Chunk!");
  delay(SECOND);

  memset(page->chunk, 97, CHUNK_SIZE);
  page->chunk[0] = 0xAB;
  page->chunk[CHUNK_SIZE - 2] = 0xFF;
  page->chunk[CHUNK_SIZE - 1] = 0xAA;
}

void halt() {
  cli();
  while (1) {
  }
}

void halt_msg(char *msg) {
  clear_screen();
  oled.print(msg);
  halt();
}

void oom(char *message) {
  cli();  // disable interrupts

  clear_screen();
  oled.setTextColor(COLOR_RED);
  oled.println("OOM: ");
  oled.println(message);

  halt();
}

void no_pages() {
  // we dont need to overwrite what we printed :)
  // that causes the lcd to blink
  if (no_page == 0) {
    clear_screen();
    oled.setTextColor(COLOR_WHITE);
    oled.print(" Waiting for messages");
    no_page = 1;
  }
  return;
}
void setup() {
  // pinMode(PIN_RESET_RX_BUFF, INPUT);
  Serial.begin(9600);

  delay(100);
  oled.begin();
  oled.setFont();
  oled.fillScreen(COLOR_BLACK);
  oled.setTextColor(COLOR_WHITE);
  oled.setTextSize(1);
  clear_screen();


  // all new pages are links
  head = (Page *)malloc(sizeof(Page));
  if (head == NULL)
    oom("Setup head");
  head->next = NULL;
  head->prev = NULL;
  head->write_ptr = 0;
  head->page_number = 0;
  alloc_empty_chunk(head);

  Page *link = (Page *)malloc(sizeof(Page));
  alloc_empty_chunk(link);
  if (link == NULL)
    oom("Setup first link");
  add_page(link);
}


void loop() {
  Serial.write('a');
  // if we have something to read, we can go ahead and read it into pages
  if (Serial.available() > 0) {

    while (Serial.available() > 0) {
      int rx_byte = Serial.read();

      clear_screen();
      if (chunk_full(head->prev) == FULL) {
        clear_screen();
        oled.println(head->prev->page_number);
        oled.println(head->prev->write_ptr);
        oled.println("");
        halt_msg("chunk full");
      }

      head->prev->chunk[head->prev->write_ptr] = rx_byte;
      head->prev->write_ptr++;
      // 0xAA is a chunk marker, kind of like a EOF in linux
      head->prev->chunk[head->prev->write_ptr] = 0xAA;
    }
    clear_screen();
    oled.print("finished reading serial data!");
    delay(SECOND);
  }

  if (head->prev->chunk[0] == 0xAB) {
    clear_screen();
    oled.print("No chunks written");
    delay(SECOND/2);
    return;
  }

  if (head->next != NULL)
    current_lk = head->next;

  while (1) {
    clear_screen();
    // draw_block(current_lk);
    for (size_t i = 0; i < CHUNK_SIZE; i++) {
      clear_screen();

      // weve hit the end of what we can read in the chunk
      if (current_lk->chunk[i] == 0xAA) 
        break;

      if (current_lk->chunk[i] != 0xFF ){
        oled.print(current_lk->chunk[i]);
        delay(2*SECOND);
      }
      clear_screen();
      oled.print(current_lk->page_number);
    }

    if (current_lk->next != NULL && current_lk->next->chunk[0] != 0xAB)
      current_lk = current_lk->next;

    delay(SECOND);
  }
}
