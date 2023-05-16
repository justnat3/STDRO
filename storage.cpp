#include <Arduino.h>
#include "storage.hpp"

struct owsd_page* alloc_owsd_page(uint8_t chunk[]) {
    struct owsd_page *new_page =
        (struct owsd_page*) malloc(sizeof(struct owsd_page*));

    if (new_page == NULL) return NULL;

    new_page->chunk = (uint8_t*)malloc(sizeof(uint8_t*)*CHUNK_SIZE);
    new_page->next = NULL;
    new_page->prev = NULL;
    return NULL;
}
