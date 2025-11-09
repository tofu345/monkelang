#pragma once

// This module contains a the definition for Table Objects, a specialized
// hash-table with Object keys and values from
// https://github.com/tofu345/hash-table.

#include <stddef.h>
#include <stdint.h>

#include "object.h"

// Number of elements in a bucket and initial number of buckets.
#define N 8

bool hashable(Object key);

// `k_type[i]` is [o_Null] if empty
typedef struct table_bucket {
    uint64_t hashes[N]; // for faster comparisons.

    // Separated [ObjectType] and [ObjectData] to avoid excessive padding.
    ObjectType k_type[N];
    ObjectData k_data[N];
    ObjectType v_type[N];
    ObjectData v_data[N];

    struct table_bucket *overflow;
} table_bucket;

struct Table {
    size_t length; // number of filled entries

    table_bucket *buckets;
    size_t _buckets_length;
};

// returns NULL or err.
void *table_init(Table *tbl);
void table_free(Table *tbl);

// Get item with [key]. Return value, or [o_Null Object] if not found.
Object table_get(Table *tbl, Object key);

// Returns [Null Object] if:
// - [key] or [value] is [Null Object].
// - allocation of [table_bucket] overflow failed.
// Returns previous value of [key] if present.
// Returns [value] otherwise.
Object table_set(Table *tbl, Object key, Object value);

// Remove item with [key] and return its value or [Null Object] if not found.
Object table_remove(Table *tbl, Object key);

typedef struct {
    Object cur_key;
    Object cur_val;

    // Don't use these fields directly.
    Table *_tbl;
    table_bucket *_bucket; // current bucket under inspection
    size_t _bucket_idx;    // index of current bucket into `_buckets`
    int _index;            // index of current entry into `_bucket.entries`
} tbl_it;

// Initialize new iterator (for use with table_next).
void tbl_iterator(tbl_it *it, Table *tbl);

// Move iterator to next item in table, update iterator's [cur_key] and
// [cur_val] current item, and return true.  If there are no more items, return
// false.  Do not mutate the table during iteration.
bool tbl_next(tbl_it *it);
