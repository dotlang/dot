#ifndef __HASH_H__
#define __HASH_H__

#ifndef _XOPEN_SOURCE 
#define _XOPEN_SOURCE 500 /* Enable certain library functions (strdup) on linux.  See feature_test_macros(7) */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>


typedef struct entry_s {
	char *key;
	void *value;
	struct entry_s *next;
} entry_t;

typedef struct hashtable_s {
	int size;
	struct entry_s **table;	
} hashtable_t;

/* Create a new hashtable. */
hashtable_t *ht_create( int size );

/* Add to hash table */
int ht_set( hashtable_t *hashtable, char *key, void *value );

/* Read from hash table */
void *ht_get( hashtable_t *hashtable, char *key );

void ht_destroy(hashtable_t* ht);

#endif
