#ifndef STRING_SET_H
#define STRING_SET_H

#include <strings.h>

typedef struct node
{
    const char *s;
    struct node *next;
} node_t;

#define SET_SIZE 65536

typedef struct str_set
{
    node_t *buckets[SET_SIZE];
    int size;
} str_set_t;

void str_set_init(str_set_t *set);
int str_set_add(str_set_t *set, const char *s);
int str_set_contains(str_set_t *set, const char *s);
int str_set_to_array(str_set_t *set, const char *array[], int max_size);
void str_set_free(str_set_t *set);
#endif