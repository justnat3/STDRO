#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#define FULL 1
#define NOT_FULL 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define SECOND 1000

#define CHUNK_MARKER 0xAA
#define CHUNK_END 0xFF
#define CHUNK_OFFSET 10
#define CHUNK_HEIGHT 11
#define CHUNK_WIDTH 7
#define CHUNK_SIZE 76

static const uint8_t PIN_SCL_SCK = 13;
static const uint8_t PIN_SDA_MOSI = 11;
static const uint8_t PIN_CS_SS = 10;
static const uint8_t PIN_RES_RST = 9;
static const uint8_t PIN_DC_RS = 8;
static const uint8_t PIN_RESET_RX_BUFF = 2;
static const uint16_t COLOR_BLACK = 0x0000;
static const uint16_t COLOR_BLUE = 0x001F;
static const uint16_t COLOR_RED = 0xF800;
static const uint16_t COLOR_GREEN = 0x07E0;
static const uint16_t COLOR_CYAN = 0x07FF;
static const uint16_t COLOR_MAGENTA = 0xF81F;
static const uint16_t COLOR_YELLOW = 0xFFE0;
static const uint16_t COLOR_WHITE = 0xFFFF;


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
  short write_cursor;
  short read_cursor;
  struct Page *next;
  struct Page *prev;
} Page;

Page *head;
Page *current_lk;

// this is just linked list append
void add_page(Page *link) {
  if (head == nullptr)
    halt_msg("no_head");

  link->page_number = head->prev->page_number += 1;
  link->write_cursor = 0;

  // at the start, we only have head so the next node become the tail
  if (head->prev == nullptr) {
    head->next = link;
    head->prev = link;
  }

  Page *tail = head->prev;
  if (tail == nullptr)
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
  head->prev->next = nullptr;  // last node next should be nullptr
  while (lk->next != nullptr) {
    lk = lk->next;
    free(lk->prev);
  }
  free(head);

}


void clear_screen() {
  reset_cursor();
  oled.fillScreen(COLOR_BLACK);
  oled.setTextColor(COLOR_WHITE);
}

char *byte_to_ascii(int byte_) {

  int size = snprintf(nullptr, 0, "%02X", byte_);
  char *fmt_str = (char *)malloc(size + 1);

  if (fmt_str == nullptr)
    oom("2chunk_to_ascii");

  sprintf(fmt_str, "%02X", byte_);
  return fmt_str;
}

int8_t chunk_full(Page *page) {
  // The chunk is not full if this position is overwritten
  if (page->chunk[CHUNK_SIZE] == CHUNK_END)
    return NOT_FULL;
  return FULL;
}

// we alloc an empty chunk with a magic marker at the end of the check
// this is so we can check if the chunk is full or not
void alloc_empty_chunk(Page *page) {

  memset(page->chunk, 0, CHUNK_SIZE);
  page->chunk[0] = CHUNK_MARKER; 
  page->chunk[CHUNK_SIZE] = CHUNK_END;
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
  if (head == nullptr)
    oom("Setup head");
  head->next = nullptr;
  head->prev = nullptr;
  head->write_cursor = 0;
  head->page_number = 0;
  alloc_empty_chunk(head);

  Page *link = (Page *)malloc(sizeof(Page));
  alloc_empty_chunk(link);
  if (link == nullptr)
    oom("Setup first link");
  add_page(link);
}

void write_to_avaliable_chunk(const int rx_byte) {
  // if we have reached the end of the page allocate a new page 
  if (head->prev->write_cursor >= CHUNK_SIZE) {
    halt_msg("needed to alloc a page");
    Page *link = (Page *) malloc(sizeof(Page));
    if (link == nullptr) 
      oom("new chunk");
    add_page(link);
  }

  if (head->prev->write_cursor >= 77) {
    clear_screen();
    oled.println(head->prev->page_number);
    oled.println(head->prev->write_cursor);
    delay(2*SECOND);
    oled.println("");
    halt_msg("chunk full");
  }

  head->prev->chunk[head->prev->write_cursor] = rx_byte;
  head->prev->write_cursor++;
  head->prev->chunk[head->prev->write_cursor] = CHUNK_MARKER;
}

short width = 0;
short height = 0;
short written = 0;

void loop() {
  // if we have something to read, we can go ahead and read it into pages
  while (Serial.available() > 0) 
    write_to_avaliable_chunk(Serial.read());


  if (current_lk == nullptr) 
    current_lk = head->next;

  // if we are at the chunk marker we dont need to read anymore
  if (current_lk->chunk[current_lk->read_cursor] == CHUNK_MARKER 
    || current_lk->chunk[0] == CHUNK_MARKER) {
    current_lk->read_cursor = 0;
    return; 
  }

  if (current_lk->read_cursor == CHUNK_SIZE-1 && current_lk->next == nullptr) {
    current_lk->read_cursor = 0;
    return;
  }

  current_lk->read_cursor++;

  //render the header
  char *header = (char *)malloc(14 * sizeof(char));
  if (header == nullptr)
    oom("draw_block");

  if (!written) {
    sprintf(header, "Chunk Page: %i", current_lk->page_number);
    oled.println(header);
    written = 1;
  }

  if (height < CHUNK_HEIGHT) {
    // set the cursor to the next row
    oled.setCursor(0, oled.getCursorY() + CHUNK_OFFSET);
  } else {
    height = 0;
  }

  char *ascii_character = byte_to_ascii(current_lk->chunk[current_lk->read_cursor]);

  if (width < CHUNK_WIDTH) {
    oled.print(ascii_character);
    oled.print(" ");
    free(&ascii_character);
  } else {
    width = 0;
  }

  // I would rather see the character
  delay(SECOND/4);
}
