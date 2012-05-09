#pragma once

#include <stdint.h>

/* Create a nutrient DB file suitable for nutrient add calls */
struct nutrient_writeable_db *nutrient_start_db(const char *filename);

/* Add a key/value pair to the DB */
int nutrient_add(struct nutrient_writeable_db *db, const uint8_t * key,
                 uint32_t key_len, const uint8_t * value, uint32_t value_len);

/* We're done writing the DB - finalise it */
int nutrient_end_db(struct nutrient_writeable_db *db);

/* Open a Nutrient DB file suitable for Nutrient find calls */
struct nutrient_readable_db *nutrient_open_db(const char *filename);

/* Find the value for a given key */
int nutrient_find(struct nutrient_readable_db *db, const uint8_t * key,
                  uint32_t key_len, uint8_t ** value, uint32_t * value_len);

/* Find the nearest match for a given name */
int nutrient_find_nearest(struct nutrient_readable_db *db,
                          const uint8_t * key, uint32_t key_len,
                          uint8_t ** nearest_key, uint32_t * nearest_key_len,
                          uint8_t ** value, uint32_t * value_len);

/* Find all values with a given prefix */
int nutrient_find_prefixed(struct nutrient_readable_db *db,
                           const uint8_t * prefix, uint32_t prefix_len);

/* Close a nutrient DB file */
int nutrient_close_db(struct nutrient_readable_db *db);
