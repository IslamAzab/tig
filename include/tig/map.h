/* Copyright (c) 2006-2014 Jonas Fonseca <jonas.fonseca@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef TIG_MAP_H
#define TIG_MAP_H

#include "tig/tig.h"
#include "tig/tig.h"

/*
 * String map.
 */

typedef unsigned int string_map_key_t;
typedef string_map_key_t (*string_map_hash_fn)(const void *value);
typedef const char *(*string_map_key_fn)(const void *value);

struct string_map {
	string_map_hash_fn hash_fn;
	string_map_key_fn key_fn;
	void *htab;
	const char *key;
};

extern string_map_hash_fn string_map_hash_helper;
void *string_map_get(struct string_map *map, const char *key);
void **string_map_put(struct string_map *map, const char *key);
void string_map_clear(struct string_map *map);

#endif
/* vim: set ts=8 sw=8 noexpandtab: */
