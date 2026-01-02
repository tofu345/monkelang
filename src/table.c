#include "table.h"
#include "object.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool hashable(Object key) {
    switch (key.type) {
        case o_String:
        case o_Boolean:
        case o_Integer:
        case o_Float:
            return true;
        default:
            return false;
    }
}

uint64_t object_hash(Object key) {
    switch (key.type) {
        case o_String:
            {
                CharBuffer *str = key.data.string;
                return hash_string_fnv1a(str->data, str->length);
            }

        case o_Boolean:
            return key.data.boolean;
        case o_Integer:
            return key.data.integer;
        case o_Float:
            return key.data.floating;

        default:
            die("object_hash: type %s (%d) not implemented",
                    show_object_type(key.type), key.type);
    }
}

static inline uint64_t
hash_index(uint64_t hash, size_t num_buckets) {
    return (hash & (uint64_t)(num_buckets - 1));
}

void *table_init(Table *tbl) {
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

void table_free(Table *tbl) {
    for (size_t i = 0; i < tbl->_buckets_length; i++) {
        destroy_overflows(tbl->buckets + i);
    }
    free(tbl->buckets);
    tbl->buckets = NULL;
    tbl->_buckets_length = 0;
}

static Object
bucket_get_value(table_bucket *bucket, uint64_t hash, ObjectType type) {
    while (bucket != NULL) {
        for (size_t i = 0; i < N; i++) {
            if (bucket->k_type[i] == o_Nothing) {
                break;
            }

            if (bucket->hashes[i] == hash && bucket->k_type[i] == type) {
                return (Object){ bucket->v_type[i], bucket->v_data[i] };
            }
        }
        bucket = bucket->overflow;
    }
    return OBJ_NOTHING;
}

Object table_get_hash(Table *tbl, Object key, uint64_t hash) {
    if (key.type == o_Nothing) return OBJ_NOTHING;
    size_t index = hash_index(hash, tbl->_buckets_length);
    return bucket_get_value(tbl->buckets + index, hash, key.type);
}

Object table_get(Table *tbl, Object key) {
    return table_get_hash(tbl, key, object_hash(key));
}

static void
_bucket_set(table_bucket *bucket, int i, Object key, Object val) {
    bucket->k_type[i] = key.type;
    bucket->k_data[i] = key.data;
    bucket->v_type[i] = val.type;
    bucket->v_data[i] = val.data;
}

static Object
bucket_set(Table *tbl, table_bucket *bucket, uint64_t hash, Object key,
        Object val) {
    for (size_t i = 0; i < N; i++) {
        if (bucket->k_type[i] == o_Nothing) {
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
        if (new_bucket == NULL) return OBJ_NOTHING;
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
table_expand(Table *tbl) {
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

    tbl_it it = tbl_iterator(tbl);
    while (tbl_next(&it)) {
        hash = it._bucket->hashes[it._index - 1]; // see [table_next]
        index = hash_index(hash, new_length);
        res = bucket_set(tbl, new_buckets + index,
                hash, it.cur_key, it.cur_val);
        if (res.type == o_Nothing) {
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

Object table_set_hash(Table *tbl, Object key, Object value, uint64_t hash) {
    if (!key.type || !value.type) { return OBJ_NOTHING; }

    // table_expand() is run when the table fills roughly half of its buckets,
    // not taking into account bucket overflows.
    //
    // Its initial max number of elements is 64 (8 buckets with 8 elements per
    // bucket, excluding overflows), will expand when tbl.length is 32.

    size_t half_full = tbl->length >= (tbl->_buckets_length * N) / 2;
    if (half_full) {
        if (!table_expand(tbl)) {
            return OBJ_NOTHING;
        }
    }

    size_t index = hash_index(hash, tbl->_buckets_length);
    return bucket_set(tbl, tbl->buckets + index, hash, key, value);
}

Object
table_set(Table *tbl, Object key, Object value) {
    return table_set_hash(tbl, key, value, object_hash(key));
}

Object
table_remove(Table *tbl, Object key) {
    if (key.type == o_Nothing) return OBJ_NOTHING;

    uint64_t hash = object_hash(key);
    size_t index = hash_index(hash, tbl->_buckets_length);
    table_bucket *bucket = tbl->buckets + index;

    while (bucket != NULL) {
        for (int i = 0; i < N; i++) {
            if (bucket->k_type[i] == o_Nothing) { break; }
            if (bucket->hashes[i] == hash && bucket->k_type[i] == key.type) {
                int last = i;
                for (; last < N - 1; last++) {
                    if (bucket->k_type[last + 1] == o_Nothing) {
                        break;
                    }
                }

                Object val = (Object){ bucket->v_type[i], bucket->v_data[i] };
                _bucket_set(bucket, i, key, val);

                // replace `Object` at [i] with [last]
                bucket->k_type[i] = bucket->k_type[last];
                bucket->k_data[i] = bucket->k_data[last];
                bucket->v_type[i] = bucket->v_type[last];
                bucket->v_data[i] = bucket->v_data[last];
                bucket->hashes[i] = bucket->hashes[last];

                bucket->k_type[last] = o_Nothing;

                tbl->length--;
                return val;
            }
        }
        bucket = bucket->overflow;
    }

    return OBJ_NOTHING;
}

tbl_it tbl_iterator(Table *tbl) {
    tbl_it it;
    it._tbl = tbl;
    it._bucket = tbl->buckets;
    it._bucket_idx = 1;
    it._index = 0;
    return it;
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
        if (it->_index >= N) {
            if (it->_bucket->overflow) {
                it->_bucket = it->_bucket->overflow;
                it->_index = 0;

            } else if (!__next_bucket(it)) {
                return false;
            }
            continue;
        }

        ObjectType key_typ = it->_bucket->k_type[it->_index];
        if (key_typ == o_Nothing) {
            if (!__next_bucket(it))
                return false;
            continue;
        }

        it->cur_key.type = key_typ;
        it->cur_key.data = it->_bucket->k_data[it->_index];
        it->cur_val.type = it->_bucket->v_type[it->_index];
        it->cur_val.data = it->_bucket->v_data[it->_index];
        it->_index++;
        return true;
    }
}
