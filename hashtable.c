// Théo Pariney
#include "hashtable.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 * value
 */

enum value_kind value_get_kind(const struct value *self) {
    return self->kind;
}

bool value_is_nil(const struct value *self) {
    return self->kind == VALUE_NIL;
}

bool value_is_boolean(const struct value *self) {
    return self->kind == VALUE_BOOLEAN;
}

bool value_is_integer(const struct value *self) {
    return self->kind == VALUE_INTEGER;
}

bool value_is_real(const struct value *self) {
    return self->kind == VALUE_REAL;
}

bool value_is_custom(const struct value *self) {
    return self->kind == VALUE_CUSTOM;
}

void value_set_nil(struct value *self) {
    self->kind = VALUE_NIL;
}

void value_set_boolean(struct value *self, bool val) {
    self->kind = VALUE_BOOLEAN;
    self->as.boolean = val;
}

void value_set_integer(struct value *self, int64_t val) {
    self->kind = VALUE_INTEGER;
    self->as.integer = val;
}

void value_set_real(struct value *self, double val) {
    self->kind = VALUE_REAL;
    self->as.real = val;
}

void value_set_custom(struct value *self, void *val) {
    self->kind = VALUE_CUSTOM;
    self->as.custom = val;
}


bool value_get_boolean(const struct value *self) {
    assert(value_is_boolean(self));
    return self->as.boolean;
}

int64_t value_get_integer(const struct value *self) {
    assert(value_is_integer(self));
    return self->as.integer;
}

double value_get_real(const struct value *self) {
    assert(value_is_real(self));
    return self->as.real;
}

void *value_get_custom(const struct value *self) {
    assert(value_is_custom(self));
    return self->as.custom;
}


struct value value_make_nil() {
    struct value res;
    value_set_nil(&res);
    return res;
}

struct value value_make_boolean(bool val) {
    struct value res;
    value_set_boolean(&res, val);
    return res;
}

struct value value_make_integer(int64_t val) {
    struct value res;
    value_set_integer(&res, val);
    return res;
}

struct value value_make_real(double val) {
    struct value res;
    value_set_real(&res, val);
    return res;
}

struct value value_make_custom(void *val) {
    struct value res;
    value_set_custom(&res, val);
    return res;
}

/*
 * Hashing function
 */
size_t hash_string(const char *str) {
    size_t hash = 0xcbf29ce484222325;
    while (str != NULL) {
        hash = hash ^ *str;
        hash = hash * 0x100000001b3;
        str++;
    }
    return hash;
}

/*
 * hashtable
 */

void hashtable_create(struct hashtable *self) {
    self->buckets = calloc(HASHTABLE_INITIAL_SIZE, sizeof(struct bucket*));
    for (size_t i = 0; i < HASHTABLE_INITIAL_SIZE; i++) self->buckets[i] = NULL;
    self->size = HASHTABLE_INITIAL_SIZE;
    self->count = 0;
}

void bucket_destroy(struct bucket *self) {
    if (self == NULL) return;
    bucket_destroy(self->next);
    free(self);
}

void hashtable_destroy(struct hashtable *self) {
    for (size_t i = 0; i < self->count; i++) bucket_destroy(self->buckets[i]);
    free(self->buckets);
}

size_t hashtable_get_count(const struct hashtable *self) {
  return self->count;
}

size_t hashtable_get_size(const struct hashtable *self) {
  return self->size;
}

char *copy_string(const char *str) {
    size_t str_length = 1;
    for (size_t i = 0; str[i] != '\0'; i++) str_length++;
    char *new = calloc(str_length, sizeof(char));
    for (size_t i = 0; i < str_length; i++) new[i] = str[i];
    return new;
}

bool string_equal(const char *str1, const char *str2) {
    size_t i = 0;
    for (; str1[i] != '\0' && str2[i] != '\0'; i++) {
        if (str1[i] != str2[i]) return false;
    }
    return str1[i] == '\0' && str2[i] == '\0';
}

void bucket_insert(struct bucket *bucket, struct bucket *inserted) {
    if (bucket->next == NULL) bucket->next = inserted;
    else bucket_insert(bucket->next, inserted);
}

bool bucket_try_replace(struct bucket *bucket, const char *key, struct value value) {
    if (bucket == NULL) return false;
    if (string_equal(bucket->key, key)) {
        bucket->value = value;
        return true;
    }
    return bucket_try_replace(bucket->next, key, value);
}

bool bucket_remove(struct bucket *bucket, const char *key) {
    if (bucket->next == NULL) return false;
    if (string_equal(bucket->next->key, key)) {
        struct bucket *removed = bucket->next;
        bucket->next = bucket->next->next;
        free(removed);
        return true;
    }
    return bucket_remove(bucket->next, key);
}

bool hashtable_insert(struct hashtable *self, const char *key, struct value value) {
    if ((double) self->count / (double) self->size >= 0.5) hashtable_rehash(self);
    size_t index = hash_string(key) % self->size;
    if (bucket_try_replace(self->buckets[index], key, value)) return false;
    struct bucket *new = malloc(sizeof(struct bucket));
    new->next = NULL;
    new->value = value;
    new->key = copy_string(key);
    if (self->buckets[index] == NULL) self->buckets[index] = new;
    else bucket_insert(self->buckets[index], new);
    self->count++;
    return true;
}

bool hashtable_remove(struct hashtable *self, const char *key) {
    size_t index = hash_string(key) % self->size;
    if (self->buckets[index] == NULL) return false;
    if (string_equal(self->buckets[index]->key, key)) {
        struct bucket *removed = self->buckets[index];
        self->buckets[index] = removed->next;
        free(removed);
        self->count--;
        return true;
    }
    bool removed = bucket_remove(self->buckets[index], key);
    if (removed) self->count--;
    return removed;
}


bool hashtable_contains(const struct hashtable *self, const char *key) {
    size_t index = hash_string(key) % self->size;
    struct bucket *curr = self->buckets[index];
    while (curr != NULL) {
        if (string_equal(curr->key, key)) return true;
        curr = curr->next;
    }
    return false;
}

void hashtable_rehash(struct hashtable *self) {
    struct bucket** new = calloc(self->size * 2, sizeof(struct bucket*));
    for (size_t i = 0; i < self->size * 2; i++) new[i] = NULL;
    for (size_t i = 0; i < self->size; i++) {
        struct bucket *curr = self->buckets[i];
        while (curr != NULL) {
            size_t index = hash_string(curr->key) % (self->size * 2);
            if (new[index] == NULL) new[index] = curr;
            else bucket_insert(new[index], curr);
            struct bucket *next = curr->next;
            curr->next = NULL;
            curr = next;
        }
    }
    free(self->buckets);
    self->size *= 2;
    self->buckets = new;
}

void hashtable_set_nil(struct hashtable *self, const char *key) {
    hashtable_insert(self, key, value_make_nil());
}

void hashtable_set_boolean(struct hashtable *self, const char *key, bool val) {
    hashtable_insert(self, key, value_make_boolean(val));
}

void hashtable_set_integer(struct hashtable *self, const char *key, int64_t val) {
    hashtable_insert(self, key, value_make_integer(val));
}

void hashtable_set_real(struct hashtable *self, const char *key, double val) {
    hashtable_insert(self, key, value_make_real(val));
}

void hashtable_set_custom(struct hashtable *self, const char *key, void *val) {
    hashtable_insert(self, key, value_make_custom(val));
}

struct value hashtable_get(struct hashtable *self, const char *key) {
    if (!hashtable_contains(self, key)) return value_make_nil();
    size_t index = hash_string(key) % self->size;
    struct bucket *curr = self->buckets[index];
    assert(curr != NULL);
    while (!string_equal(curr->key, key)) {
        curr = curr->next;
        assert(curr != NULL);
    }
    return curr->value;
}
