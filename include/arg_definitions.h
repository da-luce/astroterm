/* Contains definitions for command line arguments.
 *
 * Each line is an invocation of a macro whose arguments define a command line argument of a particular type.
 *
 * The macro arguments are:
 * - The source-code variable name of the argument definition.
 * - The short and long versions of the argument.
 * - The type of the argument (where applicable).
 * - The help text for the argument.
 *
 * To use these definitions, define the 4 macros below so that they use the arguments, then
 * include this file in the source code.
 *
 * This approach is based on the idea of "X macros" (https://en.wikipedia.org/wiki/X_macro)
 *
 */

#if !defined(INCLUDE_ARG_DEFINITION_DBL0)
    #define INCLUDE_ARG_DEFINITION_DBL0(...)
#endif

#if !defined(INCLUDE_ARG_DEFINITION_STR0)
    #define INCLUDE_ARG_DEFINITION_STR0(...)
#endif

#if !defined(INCLUDE_ARG_DEFINITION_LIT0)
    #define INCLUDE_ARG_DEFINITION_LIT0(...)
#endif

#if !defined(INCLUDE_ARG_DEFINITION_INT0)
    #define INCLUDE_ARG_DEFINITION_INT0(...)
#endif

INCLUDE_ARG_DEFINITION_DBL0(latitude_arg, "a", "latitude", "<degrees>", "Observer latitude [-90°, 90°] (default: 0.0)");
INCLUDE_ARG_DEFINITION_DBL0(longitude_arg, "o", "longitude", "<degrees>", "Observer longitude [-180°, 180°] (default: 0.0)");
INCLUDE_ARG_DEFINITION_DBL0(threshold_arg, "t", "threshold", "<float>", "Only render stars brighter than this magnitude (default: 5.0)");
INCLUDE_ARG_DEFINITION_DBL0(label_arg, "l", "label-thresh", "<float>", "Label stars brighter than this magnitude (default: 0.25)");
INCLUDE_ARG_DEFINITION_DBL0(speed_arg, "s", "speed", "<float>", "Animation speed multiplier (default: 1.0)");
INCLUDE_ARG_DEFINITION_DBL0(ratio_arg, "r", "aspect-ratio", "<float>", "Override the calculated terminal cell aspect ratio. Use this if your projection is not 'square.' A value around 2.0 works well for most cases");
INCLUDE_ARG_DEFINITION_STR0(datetime_arg, "d", "datetime", "<yyyy-mm-ddThh:mm:ss>", "Observation datetime in UTC");
INCLUDE_ARG_DEFINITION_STR0(city_arg, "i", "city", "<city_name>", "Use the latitude and longitude of the provided city. If the name contains multiple words, enclose the name in single or double quotes. For a list of available cities, see: https://github.com/da-luce/astroterm/blob/v" PROJ_VERSION "/data/cities.csv");
INCLUDE_ARG_DEFINITION_LIT0(color_arg, "c", "color", "Enable terminal colors");
INCLUDE_ARG_DEFINITION_LIT0(constell_arg, "C", "constellations", "Draw constellation stick figures. Note: a constellation is only drawn if all stars in the figure are over the threshold");
INCLUDE_ARG_DEFINITION_LIT0(grid_arg, "g", "grid", "Draw an azimuthal grid");
INCLUDE_ARG_DEFINITION_LIT0(unicode_arg, "u", "unicode", "Use unicode characters");
INCLUDE_ARG_DEFINITION_LIT0(quit_arg, "q", "quit-on-any", "Quit on any keypress (default is to quit on 'q' or 'ESC' only)");
INCLUDE_ARG_DEFINITION_LIT0(meta_arg, "m", "metadata", "Display metadata");
INCLUDE_ARG_DEFINITION_LIT0(help_arg, "h", "help", "Print this help message");
INCLUDE_ARG_DEFINITION_LIT0(completions_arg, "b", "bash-completions", "Print bash completions");
INCLUDE_ARG_DEFINITION_LIT0(version_arg, "v", "version", "Display version info and exit");
INCLUDE_ARG_DEFINITION_INT0(fps_arg, "f", "fps", "<int>", "Frames per second (default: 24)");

#undef INCLUDE_ARG_DEFINITION_DBL0
#undef INCLUDE_ARG_DEFINITION_STR0
#undef INCLUDE_ARG_DEFINITION_LIT0
#undef INCLUDE_ARG_DEFINITION_INT0
