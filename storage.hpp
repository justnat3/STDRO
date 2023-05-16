#include <Arduino.h>

#define CHUNK_SIZE              77
#define SIZE                    9
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   128
#define BLOCK_HEIGHT    11
#define BLOCK_WIDTH     7
#define HEIGHT_OFFSET   10

struct owsd_page
{
    uint8_t *chunk;
        uint8_t page_number;
    struct owsd_page *next, *prev;
};

#define OWSD_PAGE_INIT(name) { ((char)0), &(name), &(name) }
#define OWSD_PAGE(name) \
        struct owsd_page name = OWSD_PAGE_INIT(name)

static inline struct owsd_page* INIT_OWSD_PAGE(struct owsd_page *node)
{
    node->chunk = (uint8_t*)malloc(sizeof(uint8_t*)*CHUNK_SIZE);
        node->next = node;
        node->prev = node;
        return node;
}

static inline void fill_page(struct owsd_page* page, uint8_t *chunk_data) {
        // ??? yikes
        memcpy(page->chunk, chunk_data, CHUNK_SIZE);
}

static inline void __list_add(struct owsd_page *node,
                struct owsd_page *prev,
                struct owsd_page *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next, node;
}

static inline void list_add(struct owsd_page *current,
                            struct owsd_page *new_node)
{
    __list_add(new_node, current->prev, current->next);
}

struct owsd_page *alloc_owsd_page(char byte);
struct owsd_page *init_page(struct owsd_page);
