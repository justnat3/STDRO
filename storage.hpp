#include <Arduino.h>
struct owsd_page
{
    char data;
    struct owsd_page *next, *prev;
};
#define OWSD_PAGE_INIT(name) { ((char)0), &(name), &(name) }
#define OWSD_PAGE(name) \
	struct owsd_page name = OWSD_PAGE_INIT(name)

static inline void INIT_OWSD_PAGE(struct owsd_page *node)
{
    node->data = NULL;
	node->next = node;
	node->prev = node;
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