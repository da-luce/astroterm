#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "split_lines.h"

/**
 * Splits a buffer into lines.
 *
 * @param data The buffer to split (will be modified).
 * @param line_count_out Pointer to int to store the number of lines.
 * @return Array of lines, or NULL on error. Caller must free each line and the array.
 */
char **split_lines(char *data, int *line_count_out)
{
    char **lines = NULL;
    size_t line_count = 0;
    const char *line = strtok(data, "\n");
    while (line != NULL)
    {
        char *line_copy = strdup(line);
        if (line_copy == NULL)
        {
            perror("Memory allocation failed");
            for (size_t i = 0; i < line_count; i++)
            {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }

        char **temp = realloc(lines, (line_count + 1) * sizeof(char *));
        if (temp == NULL)
        {
            perror("Memory allocation failed");
            free(line_copy);
            for (size_t i = 0; i < line_count; i++)
            {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }
        lines = temp;

        lines[line_count++] = line_copy;
        line = strtok(NULL, "\n");
    }
    if (line_count_out)
        *line_count_out = (int)line_count;
    return lines;
}
