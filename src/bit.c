#include "bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Char

char byte_to_char(uint8_t byte)
{
    return (char)byte;
}

// Fixed width types

// Signed formats

int16_t bytes_to_int16_LE(const uint8_t *buffer)
{
    int16_t result = 0x0;
    for (size_t i = 0; i < sizeof(int16_t); ++i)
    {
        result = result | (int16_t)buffer[i] << (8 * i);
    }
    return result;
}

int32_t bytes_to_int32_LE(const uint8_t *buffer)
{
    uint32_t result = 0x0;
    for (size_t i = 0; i < sizeof(int32_t); ++i)
    {
        result = result | (uint32_t)buffer[i] << (8 * i);
    }
    return result;
}

int64_t bytes_to_int64_LE(const uint8_t *buffer)
{
    uint64_t result = 0x0;
    for (size_t i = 0; i < sizeof(int64_t); ++i)
    {
        result = result | (int64_t)buffer[i] << (8 * i);
    }
    return result;
}

// Unsigned formats

uint16_t bytes_to_uint16_LE(const uint8_t *buffer)
{
    uint16_t result = 0x0;
    for (size_t i = 0; i < sizeof(uint16_t); ++i)
    {
        result = result | (uint16_t)buffer[i] << (8 * i);
    }
    return result;
}

uint32_t bytes_to_uint32_LE(const uint8_t *buffer)
{
    uint32_t result = 0x0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i)
    {
        result = result | (uint32_t)buffer[i] << (8 * i);
    }
    return result;
}

uint64_t bytes_to_uint64_LE(const uint8_t *buffer)
{
    uint64_t result = 0x0;
    for (size_t i = 0; i < sizeof(uint64_t); ++i)
    {
        result = result | (uint64_t)buffer[i] << (8 * i);
    }
    return result;
}

// Floating point formats

float bytes_to_float32_LE(const uint8_t *buffer)
{
    float f;
    uint32_t tempInt = bytes_to_uint32_LE(buffer);

    // TODO: since C89+ enforces char to be a minimum of 8 bits
    // we are guaranteed that float stores >= 4 * 8 bits?
    // Is memcpy here safe?
    memcpy(&f, &tempInt, sizeof(float));
    return f;
}

double bytes_to_double64_LE(const uint8_t *buffer)
{
    double d;

    uint64_t tempInt = bytes_to_uint64_LE(buffer);

    memcpy(&d, &tempInt, sizeof(double));
    return d;
}

// Boolean formats

bool bytes_to_bool32_LE(const uint8_t *buffer)
{
    int result = bytes_to_int32_LE(buffer);
    return (result != 0);
}
