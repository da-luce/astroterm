#ifndef CITY_H
#define CITY_H

typedef struct
{
    const char *city_name;
    float latitude;
    float longitude;
} CityData;

/* Attempt to get the coordinates of a city by name from the provided CSV
 * byte buffer. Returns NULL if not found. When multiple rows share the same
 * normalized name, the one with the highest population is returned.
 */
CityData *get_city(const char *name, const unsigned char *data, unsigned int data_len);

/* Free memory used by CityData struct.
 */
void free_city(CityData *city);

/* Apply a callback and some associated data to every city row in the provided
 * CSV byte buffer.
 */
void iter_cities(void (*callback)(const CityData *city, void *data), void *user_data, const unsigned char *data,
                 unsigned int data_len);

#endif // CITY_H
