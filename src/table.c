#include "table.h"
#include "object.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

bool hashable(Object key) {
    switch (key.type) {
        case o_String:
        case o_Boolean:
        case o_Integer:
            return true;
        default:
            return false;
    }
}

// TODO: user error instead of crash.
uint64_t object_hash(Object key) {
    uint64_t hash;
    switch (key.type) {
        case o_String:
            // hash_fnv1a
            hash = FNV_OFFSET;
            for (const char *p = key.data.string->data; *p; p++) {
                hash ^= (uint64_t)(unsigned char)(*p);
                hash *= FNV_PRIME;
            }
            return hash;

        case o_Boolean:
            return key.data.boolean;
        case o_Integer:
            return key.data.integer;

        default:
            die("object_hash: type %s (%d) not implemented", show_object_type(key.type), key.type);
            return 0;
    }
}

static inline uint64_t
hash_index(uint64_t hash, size_t num_buckets) {
    return (hash & (uint64_t)(num_buckets - 1));
}

void *table_init(table *tbl) {
    tbl->length = 0;
    tbl->_buckets_length = N;
    tbl->buckets = calloc(N, sizeof(table_bucket));
    if (tbl->buckets == NULL) {
        return NULL;
    }
    return tbl;
}

static void
destroy_overflows(table_bucket *bucket) {
    if (bucket && bucket->overflow) {
        destroy_overflows(bucket->overflow);
        free(bucket->overflow);
    }
}

void table_free(table *tbl) {
    for (size_t i = 0; i < tbl->_buckets_length; i++) {
        destroy_overflows(tbl->buckets + i);
    }
    free(tbl->buckets);
}

static Object
bucket_get_value(table_bucket *bucket, uint64_t hash, ObjectType type) {
    while (bucket != NULL) {
        for (size_t i = 0; i < N; i++) {
            if (bucket->k_type[i] == o_Null)
                return (Object){};

            if (bucket->hashes[i] == hash && bucket->k_type[i] == type)
                return (Object){ bucket->v_type[i], bucket->v_data[i] };
        }
        bucket = bucket->overflow;
    }
    return (Object){};
}

Object table_get(table *tbl, Object key) {
    if (key.type == o_Null) return (Object){};
    uint64_t hash = object_hash(key);
    size_t index = hash_index(hash, tbl->_buckets_length);
    return bucket_get_value(tbl->buckets + index, hash, key.type);
}

static void
_bucket_set(table_bucket *bucket, int i, Object key, Object val) {
    bucket->k_type[i] = key.type;
    bucket->k_data[i] = key.data;
    bucket->v_type[i] = val.type;
    bucket->v_data[i] = val.data;
}

static Object
bucket_set(table *tbl, table_bucket *bucket, uint64_t hash, Object key,
        Object val) {
    for (size_t i = 0; i < N; i++) {
        // hashes[i] is not guaranteed to not be 0 (NULL)
        if (bucket->k_type[i] == o_Null) {
            _bucket_set(bucket, i, key, val);
            bucket->hashes[i] = hash;
            tbl->length++;
            return val;

        } else if (bucket->hashes[i] == hash && bucket->k_type[i] == key.type) {
            Object old_val = (Object){ bucket->v_type[i], bucket->v_data[i] };
            bucket->v_type[i] = val.type;
            bucket->v_data[i] = val.data;
            return old_val;
        }
    }

    if (bucket->overflow != NULL)
        return bucket_set(tbl, bucket->overflow, hash, key, val);
    else {
        table_bucket *new_bucket = calloc(1, sizeof(table_bucket));
        if (new_bucket == NULL) return (Object){};
        _bucket_set(new_bucket, 0, key, val);
        new_bucket->hashes[0] = hash;
        tbl->length++;
        bucket->overflow = new_bucket;
        return val;
    }
}

// create new bucket list with double number of current buckets and
// NOTE: immediately move all elements to new buckets.
// Returns false if no memory or could not move.
static bool
table_expand(table *tbl) {
    size_t new_length = tbl->_buckets_length * 2;
    if (new_length < tbl->_buckets_length) return false; // overflow

    table_bucket *old_buckets = tbl->buckets,
              *new_buckets = calloc(new_length, sizeof(table_bucket));
    if (new_buckets == NULL) {
        tbl->buckets = old_buckets;
        return false;
    }

    // [bucket_set] auto increments
    int num_elements = tbl->length;
    tbl->length = 0;

    // TODO: expand not all at once?

    size_t index;
    uint64_t hash;
    Object res;

    tbl_it it;
    tbl_iterator(&it, tbl);
    while (tbl_next(&it)) {
        hash = it._bucket->hashes[it._index - 1]; // see [table_next]
        index = hash_index(hash, new_length);
        res = bucket_set(tbl, new_buckets + index,
                hash, it.cur_key, it.cur_val);
        if (res.type == o_Null) {
            for (size_t i = 0; i < new_length; i++) {
                destroy_overflows(new_buckets + i);
            }
            free(new_buckets);
            tbl->buckets = old_buckets;
            tbl->length = num_elements;
            return false;
        }
    }

    free(old_buckets);
    tbl->buckets = new_buckets;
    tbl->_buckets_length = new_length;
    return true;
}

Object
table_set(table *tbl, Object key, Object value) {
    if (IS_NULL(key) || IS_NULL(value)) return (Object){};

    size_t half_full = tbl->length >= (tbl->_buckets_length * N) / 2;
    if (half_full) {
        if (!table_expand(tbl)) {
            return (Object){};
        }
    }

    uint64_t hash = object_hash(key);
    size_t index = hash_index(hash, tbl->_buckets_length);
    return bucket_set(tbl, tbl->buckets + index, hash, key, value);
}

Object
table_remove(table *tbl, Object key) {
    if (key.type == o_Null) return (Object){};

    uint64_t hash = object_hash(key);
    size_t index = hash_index(hash, tbl->_buckets_length);
    table_bucket *bucket = tbl->buckets + index;

    while (bucket != NULL) {
        for (int i = 0; i < N; i++) {
            if (bucket->k_type[i] == o_Null) break;
            if (bucket->hashes[i] == hash && bucket->k_type[i] == key.type) {
                int last = i + 1;
                for (; last < N - 1; last++) {
                    if (bucket->k_type[last + 1] == o_Null) {
                        break;
                    }
                }

                Object val = (Object){ bucket->v_type[i], bucket->v_data[i] };
                _bucket_set(bucket, i, key, val);

                // swap [Objects] at [i] and [last]
                bucket->k_type[i] = bucket->k_type[last];
                bucket->k_data[i] = bucket->k_data[last];
                bucket->v_type[i] = bucket->v_type[last];
                bucket->v_data[i] = bucket->v_data[last];
                bucket->hashes[i] = bucket->hashes[last];

                bucket->k_type[last] = o_Null;
                tbl->length--;
                return val;
            }
        }
        bucket = bucket->overflow;
    }

    return (Object){};
}

void
tbl_iterator(tbl_it *it, table *tbl) {
    it->_tbl = tbl;
    it->_bucket = tbl->buckets;
    it->_bucket_idx = 1;
    it->_index = 0;
}

static bool
__next_bucket(tbl_it *it) {
    it->_index = 0;
    if (it->_bucket_idx >= it->_tbl->_buckets_length) {
        return false;
    } else {
        it->_bucket = &it->_tbl->buckets[it->_bucket_idx++];
        return true;
    }
}

bool
tbl_next(tbl_it *it) {
    while (1) {
        if (it->_index > N) {
            if (!__next_bucket(it))
                return false;
            continue;
        }

        ObjectType key_typ = it->_bucket->k_type[it->_index];
        if (key_typ == o_Null) {
            if (!__next_bucket(it))
                return false;
            continue;
        }

        it->cur_key = (Object){
            .type = key_typ,
            .data = it->_bucket->k_data[it->_index]
        };
        it->cur_val = (Object){
            .type = it->_bucket->v_type[it->_index],
            .data = it->_bucket->v_data[it->_index]
        };
        it->_index++;
        return true;
    }
}
