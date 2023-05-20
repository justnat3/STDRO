#include <Arduino.h>
#include "page.h"
#include "types.h"

#define NULL ((void*)0)

struct page *alloc_head(u8 *chunk) {

    // all new pages are links
    struct page *link = 
        (struct page*) malloc(CHUNK_SIZE * sizeof(struct page));

    link->next = NULL;
    link->prev = NULL;
    link->chunk = chunk;
    return link;
}

i8 add_page(struct page* head, u8 *chunk, u8 page_number)
{
    struct page *link = 
        (struct page*) malloc(CHUNK_SIZE * sizeof(struct page));
    
    link->page_number = page_number;
    
    if (link == NULL) return ENOMEM;
    if (head == NULL) return NO_HEAD;

    // there is no list end
    if (head->prev == NULL) {
        head->prev = link;
        head->next = link;
    }
}