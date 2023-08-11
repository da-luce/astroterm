#ifndef KEP_ELEMS_H
#define KEP_ELEMS_H

#include "../astro.h"

// Keplerian elements for orbits of the planets
// https://ssd.jpl.nasa.gov/planets/approx_pos.html *
// *Recomputed to use argument of perihelion & mean anomaly

static const struct kep_elems planet_elements[NUM_PLANETS] =
{
    [MERCURY]   = {0.38709843,	    0.20563661,     7.00559432,     174.79394829,	29.11810076,    48.33961819     },
    [VENUS]     = {0.72332102,	    0.00676399,     3.39777545,	    50.21215137,	55.09494217,	76.67261496     },
    [EARTH]     = {1.00000018,	    0.01673163,	    -0.00054346,    -2.46314313,	108.04266274,	-5.11260389     },
    [MARS]      = {1.52371243,	    0.09336511,	    1.85181869,	    19.34931620,	-73.63065768,	49.71320984     },
    [JUPITER]   = {5.20248019,	    0.04853590,	    1.29861416,	    20.05983908,	-86.01787410,	100.29282654    },
    [SATURN]    = {9.54149883,	    0.05550825,	    2.49424102,	    -42.78564734,	-20.77862639,	113.63998702    },
    [URANUS]    = {19.18797948,	    0.04685740,	    0.77298127,	    141.76872184,	98.47154226,	73.96250215     },
    [NEPTUNE]   = {30.06952752,     0.00895439,	    1.77005520,	    257.54130563,   -85.10477129,	131.78635853    }
};

static const struct kep_rates planet_rates[NUM_PLANETS] =
{
    [MERCURY]   = {0.00000000,	    0.00002123,	    -0.00590158,	149472.51546610,    0.28154195,     -0.12214182     },
    [VENUS]     = {-0.00000026,	    -0.00005107,	0.00043494,	    58517.75880612,	    0.32953822,     -0.27274174     },
    [EARTH]     = {-0.00000003,	    -0.00003661,	-0.01337178,	35999.05511069,	    0.55919116,     -0.24123856     },
    [MARS]      = {0.00000097,	    0.00009149,     -0.00724757,	19139.84710618,	    0.72076056,     -0.26852431     },
    [JUPITER]   = {-0.00002864,	    0.00018026,	    -0.00322699,	3034.72172561,	    0.05174577,     0.13024619      },
    [SATURN]    = {-0.00003065,	    -0.00032044,	0.00451969,	    1221.57315246,	    0.79194480,     -0.25015002     },
    [URANUS]    = {-0.00020455,     -0.00001550,	-0.00180155,	428.40245610,	    0.03527286,     0.05739699      },
    [NEPTUNE]   = {0.00006447,	    0.00000818,	    0.00022400,     218.45505376,	    0.01616240,     -0.00606302     }
};

static const struct kep_extra planet_extras[NUM_PLANETS] =
{
    [JUPITER]   = {-0.00012452,     0.06064060,     -0.35635438,    38.35125000 },
    [SATURN]    = {0.00025899,      -0.13434469,    0.87320147,     38.35125000 },
    [URANUS]    = {0.00058331,      -0.97731848,    0.17689245,     7.67025000  },
    [NEPTUNE]   = {-0.00041348,     0.68346318,     -0.10162547,    7.67025000  }
};

// Keplerian elements for orbits of the planets
// https://ssd.jpl.nasa.gov/planets/approx_pos.html *

// *Recomputed to use AU instead of Earth radii and rates in deg/cent vs. deg/day
// static const struct kep_elems moon_elements = {0.00256948,  0.054900,   5.1454,     115.3654,           318.0634,       125.1228        };
// static const struct kep_rates moon_rates    = {0.0,         0.0,        0.0,        476872.24270785,    5999.04226395,  1932.81400295   };
// static const struct kep_elems moon_elements = {.a = 60.2666, .e = 0.054900, .I = 5.1454, .M = 115.3654, .w = 318.0634, .O = 125.1228};
// static const struct kep_rates moon_rates = {.da = 0.0, .de = 0.0, .dI = 0.0, .dM = 13.0649929509, .dw = 0.1643573223, .dO = -0.0529538083};
static const struct kep_elems moon_elements = {.a = 60.2666, .e = 0.054900, .I = 5.1454, .M = 115.3654, .w = 318.0634, .O = 125.1228};
static const struct kep_rates moon_rates = {.da = 0.0, .de = 0.0, .dI = 0.0, .dM = 13.0649929509, .dw = 0.1643573223, .dO = -0.0529538083};

#endif // KEP_ELEMS_H