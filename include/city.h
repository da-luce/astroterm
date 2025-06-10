#ifndef CITY_H
#define CITY_H

typedef struct
{
    const char *city_name;
    float latitude;
    float longitude;
} CityData;

/* Attempt to get the coordinates of a city by name. Returns NULL if not found.
 */
CityData *get_city(const char *name);

/* Free memory used by CityData struct.
 */
void free_city(CityData *city);

/* Apply a callback and some associated data to all city definitions
 */
void iter_cities(void (*callback)(const CityData *city, void *data), void *data);

#endif // CITY_H
