#include "string_set.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint16_t str_hash(const char *s)
{
    uint16_t hash = 0;

    int n = strlen(s);
    for (int i = 0; i < n; i++)
    {
        hash += s[i] << 1;
    }
    return hash;
}

void str_set_init(str_set_t *set)
{
    for (int i = 0; i < SET_SIZE; i++)
        set->buckets[i] = NULL;
}

int str_set_contains_hash(str_set_t *set, const char *s, uint16_t hash)
{
    for (node_t *curr = set->buckets[hash]; curr != NULL; curr = curr->next)
    {
        if (strcmp(curr->s, s) == 0)
            return 1;
    }
    return 0;
}

int str_set_add(str_set_t *set, const char *s)
{
    int hash = str_hash(s);
    if (str_set_contains_hash(set, s, hash))
        return 0;

    node_t *node = malloc(sizeof(node_t));
    node->next = set->buckets[hash];
    node->s = s;
    set->buckets[hash] = node;
    set->size++;
}

int str_set_contains(str_set_t *set, const char *s)
{
    return str_set_contains_hash(set, s, str_hash(s));
}

void str_set_free(str_set_t *set)
{
    for (int i = 0; i < SET_SIZE; i++)
    {
        for (node_t *curr = set->buckets[i]; curr != NULL;)
        {
            node_t *next = curr->next;
            free(curr);
            curr = next;
        }
    }
}
