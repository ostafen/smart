#include "string_set.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint16_t str_hash(const char *s)
{
    uint16_t hash = 0;

    size_t n = strlen(s);
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
    node->allocated_string = NULL;
    set->buckets[hash] = node;
    set->size++;

    return 1;
}

int str_set_add_copy(str_set_t *set, const char *s)
{
    int hash = str_hash(s);
    if (str_set_contains_hash(set, s, hash))
        return 0;

    node_t *node = malloc(sizeof(node_t));
    size_t length = strlen(s) + 1;
    node->allocated_string = malloc(sizeof(char) * length);
    memcpy(node->allocated_string, s, length);
    node->next = set->buckets[hash];
    node->s = node->allocated_string;
    set->buckets[hash] = node;
    set->size++;

    return 1;
}

int str_set_contains(str_set_t *set, const char *s)
{
    return str_set_contains_hash(set, s, str_hash(s));
}

int str_set_to_array(str_set_t *set, const char *array[], int max_size)
{
    int num_added = 0;
    for (int i = 0; i < SET_SIZE; i++)
    {
        for (node_t *curr = set->buckets[i]; curr != NULL;)
        {
            if (num_added >= max_size)
                break;
            array[num_added++] = curr->s;
            curr = curr->next;
        }
    }

    return num_added;
}

void str_set_free(str_set_t *set)
{
    for (int i = 0; i < SET_SIZE; i++)
    {
        for (node_t *curr = set->buckets[i]; curr != NULL;)
        {
            node_t *next = curr->next;
            if (curr->allocated_string != NULL) free(curr->allocated_string);
            free(curr);
            curr = next;
        }
    }
}
