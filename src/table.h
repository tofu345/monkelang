#pragma once

// specialized hash-table (from https://github.com/tofu345/hash-table) for [Object]'s

#include <stddef.h>
#include <stdint.h>

#include "object.h"

// Number of elements in a bucket and initial number of buckets.
#define N 8

bool hashable(Object key);

// Separated [ObjectType] and [ObjectData] to avoid excessive padding.
typedef struct {
    ObjectData key;
    ObjectData val;
} table_entry;

// `k_type[i]` is [o_Null] if empty
typedef struct table_bucket {
    uint64_t hashes[N]; // for faster comparisons.
    ObjectType k_type[N];
    ObjectType v_type[N];
    table_entry entries[N];
    struct table_bucket *overflow;
} table_bucket;

struct table {
    size_t length; // number of filled entries

    table_bucket *buckets;
    size_t _buckets_length;
};

// returns NULL or err.
void *table_init(table *tbl);
void table_free(table *tbl);

// Get item with [key].
// Return value, or [Object] with type [o_Null] if not found.
Object table_get(table *tbl, Object key);

// Returns:
// - previous value of [key] if present.
// - [value] on success, otherwise
// - [Object] with [o_Null] type.
Object table_set(table *tbl, Object key, Object value);

// Remove item with [key] and return its value or [o_Null] if not found.
Object table_remove(table *tbl, Object key);

typedef struct {
    Object cur_key;
    Object cur_val;

    // Don't use these fields directly.
    table *_tbl;
    table_bucket *_bucket;  // current bucket under inspection
    size_t _bucket_idx;     // index of current bucket into `_buckets`
    int _index;          // index of current entry into `_bucket.entries`
} tbl_it;

// Initialize new iterator (for use with table_next).
void tbl_iterator(tbl_it *it, table *tbl);

// Move iterator to next item in table, update iterator's current
// [table_entry] to current item, and return true. If there are no more items,
// return false. Do not mutate the table during iteration.
bool tbl_next(tbl_it *it);
