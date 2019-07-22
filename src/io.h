#ifndef IO_H
#define IO_H

#ifdef __cplusplus
extern "C" {
#endif
/*
 * These are endian-independant routines for reading and writing
 * 4-byte (int), 2-byte (short), and 1-byte (char) values to and from
 * the ultima 4 data files.
 */

int writeInt(uint32_t i, FILE *f);
int writeShort(uint16_t s, FILE *f);
int writeChar(uint8_t c, FILE *f);
int readInt(uint32_t *i, FILE *f);
int readShort(uint16_t *s, FILE *f);
int readChar(uint8_t *c, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
