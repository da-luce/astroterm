/* Core functions for parsing data and data structures
 */

#ifndef CORE_H
#define CORE_H

#include "astro.h"
#include "parse_BSC5.h"

#include <stdbool.h>
#include <time.h>

/* Describes how objects should be rendered
 */
struct Conf
{
    double longitude;
    double latitude;
    const char *dt_string_utc;
    float threshold;
    float label_thresh;
    int fps;
    float speed;
    double julian_date;
    double aspect_ratio;
    bool quit_on_any;
    bool unicode;
    bool color;
    bool grid;
    bool constell;
    bool metadata;
};

// All information pertinent to rendering a celestial body
struct ObjectBase
{
    double azimuth; // Coordinates used for rendering
    double altitude;
    int color_pair; // 0 indicates no color pair
    char symbol_ASCII;
    const char *symbol_unicode;
    const char *label;
};

struct Star
{
    struct ObjectBase base;
    int catalog_number;
    double right_ascension;
    double declination;
    double ra_motion;
    double dec_motion;
    float magnitude;
};

struct Planet
{
    struct ObjectBase base;
    const struct KepElems *elements;
    const struct KepRates *rates;
    const struct KepExtra *extras;
    float magnitude;
};

struct Moon
{
    struct ObjectBase base;
    const struct KepElems *elements;
    const struct KepRates *rates;
    float magnitude;
};

struct Constell
{
    unsigned int num_segments;
    int *star_numbers;
};

struct StarName
{
    char *name;
};

// Data structure generation

/* Fill array of star structures using entries from BSC5 and table of star
 * names. Stars with catalog number `n` are mapped to index `n-1`. This function
 * allocates memory which must be freed by the caller. Returns false upon memory
 * allocation error
 */
bool generate_star_table(struct Star **star_table, struct Entry *entries, const struct StarName *name_table,
                         unsigned int num_stars);

/* Parse data from bsc5_names.txt and return an array of names. Stars with
 * catalog number `n` are mapped to index `n-1`. This function allocates memory
 * which should be freed by the caller. Returns false upon memory allocation
 * error.
 */
bool generate_name_table(const uint8_t *data, size_t data_len, struct StarName **name_table_out, int num_stars);

/* Parse data from bsc5_constellations.txt and return an array of constell
 * structs. This function allocates memory which should  be freed by the
 * caller. Returns false upon memory allocation error.
 *
 * NOTE: bsc5.constellations.txt MUST end in a new line to grab all the data.
 */
bool generate_constell_table(const uint8_t *data, size_t data_len, struct Constell **constell_table_out,
                             unsigned int *num_constell_out);

/* Generate an array of planet structs. This function allocates memory which
 * should  be freed by the caller. Returns false upon memory allocation error
 */
bool generate_planet_table(struct Planet **planet_table, const struct KepElems *planet_elements,
                           const struct KepRates *planet_rates, const struct KepExtra *planet_extras);

/* Generate a moon struct. Returns false upon error during generation
 */
bool generate_moon_object(struct Moon *moon_data, const struct KepElems *moon_elements, const struct KepRates *moon_rates);

// Memory freeing

void free_stars(struct Star *star_table, unsigned int size);
void free_star_names(struct StarName *name_table, unsigned int size);
void free_constells(struct Constell *constell_table, unsigned int size);
void free_planets(struct Planet *planets, unsigned int size);
void free_moon_object(struct Moon moon_data);

// Miscellaneous

/* Comparator for star structs
 */
int star_magnitude_comparator(const void *v1, const void *v2);

/* Modify an array of star numbers sorted by increasing magnitude. Used in
 * rendering functions so brighter stars are always rendered on top
 */
bool star_numbers_by_magnitude(int **num_by_mag, const struct Star *star_table, unsigned int num_stars);

/* Map a double `input` which lies in range [min_float, max_float]
 * to an integer which lies in range [min_int, max_int].
 */
int map_float_to_int_range(double min_float, double max_float, int min_int, int max_int, double input);

/* Parse a string in format yyy-mm-ddThh:mm:ss to a tm struct. Returns false
 * upon error during conversion
 */
bool string_to_time(const char *string, struct tm *time);

/* Convert elapsed time in days into years, days, hours, minutes, and seconds.
 * The input represents total elapsed time as a double (in days), and the
 * results are stored in the provided integer pointers.
 */
void elapsed_time_to_components(double elapsed_days, int *years, int *days, int *hours, int *minutes, int *seconds);

#endif // CORE_H
