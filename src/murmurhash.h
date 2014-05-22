/*
 * Via http://code.google.com/p/smhasher/:
 *
 * "All MurmurHash versions are public domain software, and the 
 * author disclaims all copyright to their code."
 */

#ifndef __MURMURHASH_H__
#define __MURMURHASH_H__

uint32_t murmurhash2(const void *key, size_t len, uint32_t seed);

#endif
