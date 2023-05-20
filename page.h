#include <Arduino.h>
#include "types.h"

#define ENOMEM      -1
#define CHUNK_SIZE 	77
#define NO_HEAD     -2

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
    u8 *chunk;
	u8 page_number;
    struct page *next, *prev;
};

i8 add_page(struct page* head, u8 *chunk, u8 page_number);
struct page *alloc_head(u8 *chunk);