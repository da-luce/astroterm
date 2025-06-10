#include "city.h"
#include "cities.h"
#include "macros.h"
#include "split_lines.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

// Function to normalize a city name: trim spaces and convert to lowercase
char *normalize_city_name(const char *input)
{
    if (input == NULL)
    {
        return NULL;
    }

    // Create a mutable copy of the input string
    char *normalized = strdup(input);
    if (normalized == NULL)
    {
        perror("Memory allocation failed");
        return NULL;
    }

    // Trim leading spaces
    char *start = normalized;
    while (isspace((unsigned char)*start))
    {
        start++;
    }

    // Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end >= start && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    // Convert to lowercase
    for (char *p = start; *p; p++)
    {
        *p = tolower((unsigned char)*p);
    }

    // Create a new string containing the normalized result
    char *result = strdup(start);
    free(normalized);
    return result;
}

// Function to compare two city names for binary search
static int compare_city(const void *key, const void *element)
{
    const char *city_name = (const char *)key;
    const char *line = *(const char **)element; // Dereference the element pointer (line array)

    // Extract city name from the line
    char temp_line[MAX_LINE_LENGTH];
    strncpy(temp_line, line, MAX_LINE_LENGTH);
    temp_line[MAX_LINE_LENGTH - 1] = '\0'; // Ensure null termination

    const char *token = strtok(temp_line, ","); // Extract the first token (city name)
    if (token == NULL)
    {
        return -1;
    }

    // Normalize both the input key and the extracted city name for comparison
    char *normalized_city_name = normalize_city_name(city_name);
    char *normalized_token = normalize_city_name(token);

    if (normalized_city_name == NULL || normalized_token == NULL)
    {
        free(normalized_city_name);
        free(normalized_token);
        return -1;
    }

    int result = strcmp(normalized_city_name, normalized_token);

    // Free allocated memory for normalized strings
    free(normalized_city_name);
    free(normalized_token);

    return result;
}

CityData *get_city(const char *name)
{
    if (name == NULL || cities_len == 0)
    {
        return NULL;
    }

    // Normalize the input city name
    char *normalized_name = normalize_city_name(name);
    if (normalized_name == NULL)
    {
        return NULL;
    }

    char *data = malloc(cities_len + 1);
    if (data == NULL)
    {
        free(normalized_name);
        return NULL;
    }

    memcpy(data, cities, cities_len);
    data[cities_len] = '\0';

    // Convert the byte array into an array of lines
    int line_count = 0;
    char **lines = split_lines(data, &line_count);
    if (lines == NULL)
    {
        free(normalized_name);
        free(data);
        return NULL;
    }

    // Perform binary search with the normalized name
    char **result = bsearch(normalized_name, lines, line_count, sizeof(char *), compare_city);

    CityData *city = NULL;
    if (result != NULL)
    {
        // Parse the line for city data
        char *matched_line = *result;
        const char *city_name, *latitude_str, *longitude_str;

        city_name = strtok(matched_line, ",");
        strtok(NULL, ","); // Skip population
        strtok(NULL, ","); // Skip country code
        strtok(NULL, ","); // Skip timezone
        latitude_str = strtok(NULL, ",");
        longitude_str = strtok(NULL, ",");

        if (city_name && latitude_str && longitude_str)
        {
            city = malloc(sizeof(CityData));
            if (city == NULL)
            {
                perror("Memory allocation failed");
            }
            else
            {
                city->city_name = strdup(city_name);
                city->latitude = atof(latitude_str);
                city->longitude = atof(longitude_str);
            }
        }
    }

    // Clean up lines and data
    for (int i = 0; i < line_count; i++)
    {
        free(lines[i]);
    }
    free(lines);
    free(data);
    free(normalized_name);

    return city;
}

void free_city(CityData *city)
{
    if (city != NULL)
    {
        free((void *)city->city_name);
        free(city);
    }
}

/**
 * @brief Iterates over all cities and applies a callback function to each.
 *
 * This function traverses the collection of cities and calls the provided
 * callback function for each city. The callback receives a pointer to the
 * city's data and a user-defined data pointer.
 *
 * @param callback A function pointer to be called for each city.
 * @param user_data A pointer to user-defined data that will be passed to the
 *                  callback function for each city.
 */
void iter_cities(void (*callback)(const CityData *city, void *data), void *user_data)
{
    if (callback == NULL)
    {
        return;
    }

    // Get the city data as a buffer using split_lines
    char *data = malloc(cities_len + 1);
    memcpy(data, cities, cities_len);
    data[cities_len] = '\0';

    int line_count = 0;
    char **lines = split_lines(data, &line_count);
    if (lines == NULL)
    {
        free(data);
        return;
    }

    for (int i = 1; i < line_count; i++)
    {
        char *line = lines[i];
        const char *city_name, *latitude_str, *longitude_str;

        city_name = strtok(line, ",");
        strtok(NULL, ","); // Skip population
        strtok(NULL, ","); // Skip country code
        strtok(NULL, ","); // Skip timezone
        latitude_str = strtok(NULL, ",");
        longitude_str = strtok(NULL, ",");

        if (city_name && latitude_str && longitude_str)
        {
            CityData city_data;
            city_data.city_name = city_name;
            city_data.latitude = atof(latitude_str);
            city_data.longitude = atof(longitude_str);

            callback(&city_data, user_data);
        }
    }
}
