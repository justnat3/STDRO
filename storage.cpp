#include <Arduino.h>
#include "storage.hpp"

struct owsd_page* alloc_owsd_page(char byte) {
    struct owsd_page *new_page = 
        (struct owsd_page*) malloc(sizeof(struct owsd_page*));

    if (new_page == NULL) return NULL;

    new_page->data = byte;
    new_page->next = NULL;
    new_page->prev = NULL;
    return new_page;
}
