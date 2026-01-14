#include "core.h"

#include "astro.h"
#include "parse_BSC5.h"
#include "strptime.h"
#include "macros.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Data generation

bool generate_star_table(struct Star **star_table_out, struct Entry *entries, const struct StarName *name_table,
                         unsigned int num_stars)
{
    *star_table_out = malloc(num_stars * sizeof(struct Star));
    if (*star_table_out == NULL)
    {
        printf("Allocation of memory for star table failed\n");
        return false;
    }

    for (unsigned int i = 0; i < num_stars; ++i)
    {
        struct Star temp_star;

        temp_star.catalog_number = (int)entries[i].XNO;
        temp_star.right_ascension = entries[i].SRA0;
        temp_star.declination = entries[i].SDEC0;
        temp_star.ra_motion = (double)entries[i].XRPM;
        temp_star.dec_motion = (double)entries[i].XDPM;
        temp_star.magnitude = entries[i].MAG / 100.0f;

        // Star magnitude mapping
        // FIXME: some of these characters render on WSL while not on macOS
        // (system wide, not just this project). I haven't gotten to the bottom
        // of this yet...
        // TODO: add CLI option to choose between these
        const char *mag_map_unicode_round[10] = {"⬤", "●", "⦁", "•", "•", "∙", "⋅", "⋅", "⋅", "⋅"};
        // const char *mag_map_unicode_diamond[10] = {"⯁", "◇", "⬥", "⬦", "⬩",
        // "🞘", "🞗", "🞗", "🞗", "🞗"}; const char *mag_map_unicode_open[10]    =
        // {"✩", "✧", "⋄", "⭒", "🞝", "🞝", "🞝", "🞝", "🞝", "🞝"}; const char
        // *mag_map_unicode_filled[10]  = {"★", "✦", "⬩", "⭑", "🞝", "🞝", "🞝",
        // "🞝", "🞝", "🞝"};
        const char mag_map_round_ASCII[10] = {'0', '0', 'O', 'O', 'o', 'o', '.', '.', '.', '.'};

        const float min_magnitude = -1.46f;
        const float max_magnitude = 7.96f;

        int symbol_index = map_float_to_int_range(min_magnitude, max_magnitude, 0, 9, temp_star.magnitude);

        temp_star.base = (struct ObjectBase){
            .color_pair = 0,
            .symbol_ASCII = (char)mag_map_round_ASCII[symbol_index],
            .symbol_unicode = mag_map_unicode_round[symbol_index],
            .label = name_table[i].name,
        };

        // Copy temp struct to table index
        (*star_table_out)[i] = temp_star;
    }

    return true;
}

bool generate_planet_table(struct Planet **planet_table, const struct KepElems *planet_elements,
                           const struct KepRates *planet_rates, const struct KepExtra *planet_extras)
{
    *planet_table = malloc(NUM_PLANETS * sizeof(struct Planet));
    if (*planet_table == NULL)
    {
        return false;
    }

    const char *planet_symbols_unicode[NUM_PLANETS] = {
        [SUN] = "☉",     [MERCURY] = "☿", [VENUS] = "♀",  [EARTH] = "🜨",  [MARS] = "♂",
        [JUPITER] = "♃", [SATURN] = "♄",  [URANUS] = "⛢", [NEPTUNE] = "♆"};

    const char planet_symbols_ASCII[NUM_PLANETS] = {
        [SUN] = '@',     [MERCURY] = '*', [VENUS] = '*',  [EARTH] = '*',  [MARS] = '*',
        [JUPITER] = '*', [SATURN] = '*',  [URANUS] = '*', [NEPTUNE] = '*'};

    const char *planet_labels[NUM_PLANETS] = {
        [SUN] = "Sun",         [MERCURY] = "Mercury", [VENUS] = "Venus",   [EARTH] = "Earth",    [MARS] = "Mars",
        [JUPITER] = "Jupiter", [SATURN] = "Saturn",   [URANUS] = "Uranus", [NEPTUNE] = "Neptune"};

    // TODO: find better way to map these values
    const int planet_colors[NUM_PLANETS] = {
        [SUN] = 4, [MERCURY] = 8, [VENUS] = 4, [MARS] = 2, [JUPITER] = 6, [SATURN] = 4, [URANUS] = 7, [NEPTUNE] = 5,
    };

    // TODO: compute these values...?
    const float planet_mean_mags[NUM_PLANETS] = {
        [SUN] = -26.832f,   [MERCURY] = 0.23f, [VENUS] = -4.14f, [MARS] = 0.71f,
        [JUPITER] = -2.20f, [SATURN] = 0.46f,  [URANUS] = 5.68f, [NEPTUNE] = 7.78f,
    };

    unsigned int i;
    for (i = 0; i < NUM_PLANETS; ++i)
    {
        struct Planet temp_planet;

        temp_planet.base = (struct ObjectBase){
            .symbol_ASCII = planet_symbols_ASCII[i],
            .color_pair = planet_colors[i],
            .symbol_unicode = planet_symbols_unicode[i],
            .label = planet_labels[i],
        };

        temp_planet.elements = &planet_elements[i];
        temp_planet.rates = &planet_rates[i];
        temp_planet.magnitude = planet_mean_mags[i];

        if (JUPITER <= i && i <= NEPTUNE)
        {
            temp_planet.extras = &planet_extras[i];
        }
        else
        {
            temp_planet.extras = NULL;
        }

        (*planet_table)[i] = temp_planet;
    }

    return true;
}

bool generate_moon_object(struct Moon *moon_data, const struct KepElems *moon_elements, const struct KepRates *moon_rates)
{
    moon_data->base = (struct ObjectBase){
        .symbol_ASCII = 'M',
        .symbol_unicode = "🌝︎︎",
        .label = "Moon",
        .color_pair = 0,
    };

    moon_data->elements = moon_elements;
    moon_data->rates = moon_rates;
    moon_data->magnitude = 0.0f; // TODO: fix this value

    return true;
}

// Line buffer (more than enough to store any of the names)
#define BUF_SIZE 32

// TODO: verify this catches the first and last entries
bool generate_name_table(const uint8_t *data, size_t data_len, struct StarName **name_table_out, int num_stars)
{
    *name_table_out = malloc(num_stars * sizeof(struct StarName));
    if (*name_table_out == NULL)
    {
        printf("Allocation of memory for name table failed\n");
        return false;
    }

    char buffer[BUF_SIZE];
    size_t offset = 0;

    // Fill array with NULL pointers to start
    for (int i = 0; i < num_stars; ++i)
    {
        (*name_table_out)[i].name = NULL;
    }

    // Set desired indices with names
    while (offset < data_len)
    {
        // Find the next line in the embedded data (similar to fgets)
        size_t i = 0;
        while (offset + i < data_len && data[offset + i] != '\n' && i < BUF_SIZE - 1)
        {
            buffer[i] = data[offset + i];
            i++;
        }
        buffer[i] = '\0'; // Null terminate the string

        // If we haven't reached the end of the buffer, move to the next line
        offset += i + 1; // Move past the newline character

        if (buffer[0] == '\0')
        {
            continue; // Skip empty lines
        }

        // Split by delimiter (expecting the format "catalog_number,name")
        int catalog_number = atoi(strtok(buffer, ","));
        const char *name = strtok(NULL, ",\n");

        int table_index = catalog_number - 1;

        struct StarName temp_name;
        temp_name.name = malloc(strlen(name) + 1);
        if (temp_name.name == NULL)
        {
            return false;
        }
        strcpy(temp_name.name, name);

        (*name_table_out)[table_index] = temp_name;
    }

    return true;
}

/* Parse a single constellation entry, e.g.:
 *
 * CVn 1 4915 4785
 *
 * Adds the entry *constell_table_out[line_number]:
 *
 * struct Constell
 * {
 * num_segments=1
 * int *star_numbers=[4915, 4785]
 * };
 *
 * NOTE: line numbers are 0-indexed
 */
bool parse_line(const uint8_t *data, struct Constell **constell_table_out, int line_start, int line_end, int line_number)
{
    // Validate the input range
    if (line_end <= line_start || data == NULL || constell_table_out == NULL)
    {
        return false;
    }

    // Create a temporary buffer for the line data
    size_t line_length = line_end - line_start + 1; // +1 to include the last character

    // #91: use dynamic allocation to avoid buffer overflow
    char *buffer = malloc(line_length + 1); // +1 for null terminator
    if (buffer == NULL)
    {
        return false;
    }

    memcpy(buffer, &data[line_start], line_length);
    buffer[line_length] = '\0';

    // Parse the line:
    const char *name = strtok(buffer, " "); // First token is the constellation name
    if (name == NULL)
    {
        free(buffer);
        return false; // Malformed line, no name found
    }

    // The next token is the number of segments
    const char *num_segments_str = strtok(NULL, " ");
    if (num_segments_str == NULL)
    {
        free(buffer);
        return false; // Malformed line, no number of segments
    }

    unsigned int num_segments = atoi(num_segments_str);
    if (num_segments == 0)
    {
        free(buffer);
        return false; // Invalid number of segments
    }

    // Allocate memory for the star numbers
    int *star_numbers = malloc(num_segments * 2 * sizeof(int)); // Each segment has two star numbers
    if (star_numbers == NULL)
    {
        return false; // Memory allocation failed
    }

    // Parse the star numbers (expecting num_segments * 2 star numbers)
    unsigned int i = 0;
    const char *token;
    while ((token = strtok(NULL, " ")) != NULL && i < num_segments * 2)
    {
        star_numbers[i] = atoi(token);
        ++i;
    }

    // If we didn't get enough star numbers, it's an error
    if (i != num_segments * 2)
    {
        free(star_numbers);
        free(buffer);
        return false; // Malformed line, not enough star numbers
    }

    // We're done with the buffer now
    free(buffer);

    // Allocate memory for the constellations table if necessary
    if (*constell_table_out == NULL)
    {
        *constell_table_out = malloc(sizeof(struct Constell) * (line_number + 1));
        if (*constell_table_out == NULL)
        {
            free(star_numbers);
            return false; // Memory allocation failed for the table
        }
    }
    else
    {
        // Reallocate memory to accommodate new constellations
        *constell_table_out = realloc(*constell_table_out, sizeof(struct Constell) * (line_number + 1));
        if (*constell_table_out == NULL)
        {
            free(star_numbers);
            return false; // Memory allocation failed for the table
        }
    }

    // Store the parsed constellation in the correct table location
    struct Constell temp_constell = {.num_segments = num_segments, .star_numbers = star_numbers};

    (*constell_table_out)[line_number] = temp_constell;

    return true;
}

bool generate_constell_table(const uint8_t *data, size_t data_len, struct Constell **constell_table_out,
                             unsigned int *num_constell_out)
{
    // Validate input
    if (data == NULL || constell_table_out == NULL || num_constell_out == NULL || data_len == 0)
    {
        return false;
    }

    unsigned int num_constells = 0;
    size_t line_start = 0;
    size_t line_end;

    // Count the number of lines in the data
    for (size_t i = 0; i < data_len; ++i)
    {
        if (data[i] == '\n')
        {
            num_constells++;
        }
    }

    // Allocate memory for the constellation table
    *constell_table_out = malloc(num_constells * sizeof(struct Constell));
    if (*constell_table_out == NULL)
    {
        printf("Allocation of memory for constellation table failed\n");
        return false;
    }

    // Parse each line of data
    int line_number = 0;
    for (size_t i = 0; i < data_len; ++i)
    {
        // Find the start of the current line
        if (i == 0 || data[i - 1] == '\n')
        {
            line_start = i;
        }

        // Find the end of the current line
        if (i == data_len - 1 || data[i] == '\n')
        {
            line_end = i;

            // Parse the line and store the parsed constellation in the table
            if (!parse_line(data, constell_table_out, line_start, line_end, line_number))
            {
                printf("Failed to parse line %d\n", line_number);
                return false;
            }

            line_number++; // Increment line number
        }
    }

    // Set the number of constellations in the output parameter
    *num_constell_out = num_constells;

    return true;
}

// Memory freeing

void free_constell_members(struct Constell constell_data)
{
    if (constell_data.star_numbers != NULL)
    {
        free(constell_data.star_numbers);
    }
    return;
}

void free_star_name_members(struct StarName name_data)
{
    if (name_data.name != NULL)
    {
        free(name_data.name);
    }
    return;
}

void free_stars(struct Star *star_table, unsigned int size)
{
    (void)size;
    free(star_table);
    return;
}

void free_planets(struct Planet *planets, unsigned int size)
{
    (void)size;
    free(planets);
    return;
}

void free_moon_object(struct Moon moon_data)
{
    // Nothing was allocated during moon generation
    (void)moon_data;
    return;
}

void free_constells(struct Constell *constell_table, unsigned int size)
{
    for (unsigned int i = 0; i < size; i++)
    {
        free_constell_members(constell_table[i]);
    }
    free(constell_table);
    return;
}

void free_star_names(struct StarName *name_table, unsigned int size)
{
    for (unsigned int i = 0; i < size; ++i)
    {
        free_star_name_members(name_table[i]);
    }
    free(name_table);
    return;
}

// Miscellaneous

int star_magnitude_comparator(const void *v1, const void *v2)
{
    const struct Star *p1 = (struct Star *)v1;
    const struct Star *p2 = (struct Star *)v2;

    // Lower magnitudes are brighter
    if (p1->magnitude < p2->magnitude)
        return +1;
    else if (p1->magnitude > p2->magnitude)
        return -1;
    else
        return 0;
}

bool star_numbers_by_magnitude(int **num_by_mag, const struct Star *star_table, unsigned int num_stars)
{
    // Create and sort a copy of the star table
    struct Star *table_copy = malloc(num_stars * sizeof(struct Star));
    if (table_copy == NULL)
    {
        printf("Allocation of memory for star table copy failed\n");
        return false;
    }

    memcpy(table_copy, star_table, num_stars * sizeof(*table_copy));
    qsort(table_copy, num_stars, sizeof(struct Star), star_magnitude_comparator);

    // Create and fill array of indices in table copy
    *num_by_mag = malloc(num_stars * sizeof(int));
    if (*num_by_mag == NULL)
    {
        printf("Allocation of memory for num by mag array failed\n");
        free(table_copy);
        return false;
    }

    for (unsigned int i = 0; i < num_stars; ++i)
    {
        (*num_by_mag)[i] = table_copy[i].catalog_number;
    }

    free(table_copy);

    return true;
}

int map_float_to_int_range(double min_float, double max_float, int min_int, int max_int, double input)
{
    double percent = (input - min_float) / (max_float - min_float);
    return min_int + (int)round((max_int - min_int) * percent);
}

bool string_to_time(const char *string, struct tm *time)
{
    const char *pointer = strptime(string, "%Y-%m-%dT%H:%M:%S", time);
    mktime(time);

    if (pointer == NULL)
    {
        return false;
    }

    return true;
}

void elapsed_time_to_components(double elapsed_days, int *years, int *days, int *hours, int *minutes, int *seconds)
{
    // Constants
    const double days_in_year = 365.25;
    const int hours_in_day = 24;
    const int minutes_in_hour = 60;
    const int seconds_in_minute = 60;

    // Calculate years
    *years = (int)(elapsed_days / days_in_year);
    double remaining_days = elapsed_days - (*years * days_in_year);

    // Calculate days
    *days = (int)remaining_days;
    double remaining_hours = (remaining_days - *days) * hours_in_day;

    // Calculate hours
    *hours = (int)remaining_hours;
    double remaining_minutes = (remaining_hours - *hours) * minutes_in_hour;

    // Calculate minutes
    *minutes = (int)remaining_minutes;
    *seconds = (int)((remaining_minutes - *minutes) * seconds_in_minute);
}
