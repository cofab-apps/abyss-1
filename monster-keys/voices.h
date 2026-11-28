/* SPDX-License-Identifier: MPL-2.0 */
enum call_type { CLACK, GROWL, CHOIR, WHALE, WHISTLE, RADIO };
struct species {
    const char *name;
    uint8_t type;
    int8_t transpose, scoop;
    uint8_t syllables, ratio, mouth, teeth, wobble, cave;
};
static const struct species creatures[] = {
    {"CRAB MONSTER",CLACK,0,15,10,7,68,100,78,25},
    {"SCP-682",GROWL,-12,-8,5,3,54,105,74,30},
    {"LERNAEAN HYDRA",CHOIR,-5,4,3,2,78,87,66,70},
    {"BAKE-KUJIRA",WHALE,-12,8,2,2,37,51,42,74},
    {"NINGEN",CHOIR,0,-5,2,1,66,42,30,79},
    {"LOCH NESS MONSTER",WHISTLE,7,-12,3,2,65,39,63,48},
    {"JACOB LEVIATHAN",RADIO,-5,12,7,5,69,85,53,70},
    {"SCYLLA",CLACK,-7,-5,12,3,43,76,95,62},
    {"TERRIBLE DOGFISH",GROWL,-5,6,6,2,82,96,67,42},
    {"THE BLOOP",WHALE,-12,16,2,1,40,47,48,54},
    {"THE DARK BLOOP",GROWL,-12,-9,3,4,30,104,65,72},
    {"EL GRAN MAJA",RADIO,-12,-5,8,3,63,88,82,62},
    {"JULIA BEAST",WHISTLE,0,11,6,3,90,88,69,54},
    {"LYNGBAKR",WHALE,-17,-4,1,2,32,35,24,86},
    {"SEA EATER",GROWL,-12,12,9,5,63,95,60,57},
    {"SCP-3000",WHISTLE,5,-7,7,4,55,71,81,48},
    {"COLOSSAL WHALE",WHALE,-12,-12,3,3,62,39,37,83},
    {"SCP-169",CLACK,-12,9,5,7,71,104,43,72},
    {"WORLD SERPENT",CHOIR,-7,-7,4,3,62,94,82,62},
    {"SPACE BLOOP",RADIO,-12,18,4,7,53,92,73,92},
};

/* Identity is audible before touching a knob. Head intervals are semitones
 * above the root. A zero count leaves room for the creature's own harmonics. */
struct signature {
    uint8_t attack_ms, decay_ms, sustain, release_ms;
    int8_t drop, semitones[3];
    uint8_t heads, spacing_ms, pulses, sub, overtone;
};
static const struct signature signature[] = {
    {2, 55,45,24,  5,{0},0,0, 9, 0,48},  /* Crab: paired chitin knocks */
    {14,140,99,53, 4,{0},0,0, 2,26,31},  /* 682: grinding jaw */
    {12,165,110,70,0,{3,7,12},3,30,0,0,8}, /* Hydra: four pitched heads */
    {4,220,63,100, 0,{0},0,0, 0,5,65},  /* Bake-Kujira: bone bell */
    {88,180,105,88,0,{12},1,65,0,0,2},   /* Ningen: breath, then halo */
    {25,130,110,48,0,{0},0,0,0,0,0},     /* Ness: rounded whistle */
    {9,72,84,55,3,{0},0,0,6,0,48},      /* Jacob: rattling plates */
    {7,110,99,58,0,{7,12},2,73,4,0,16}, /* Scylla: answering heads */
    {6,83,85,36,7,{0},0,0,5,12,48},     /* Dogfish: snapping teeth */
    {43,190,103,92,15,{0},0,0,0,20,0},  /* Bloop: falling air pocket */
    {70,210,100,96,9,{0},0,0,1,64,12},  /* Dark Bloop: low throbbing */
    {18,115,82,66,5,{0},0,0,7,28,48},   /* Maja: pulsing jaw */
    {4,90,80,54,10,{12},1,36,5,0,18},   /* Julia: bright split cry */
    {115,225,112,105,0,{0},0,0,0,56,2}, /* Lyngbakr: rising island */
    {8,72,72,48,8,{0},0,0,8,36,46},     /* Sea Eater: repeated bites */
    {64,185,111,79,2,{0},0,0,2,0,22},   /* 3000: winding reed */
    {76,215,104,112,3,{7},1,60,0,50,2}, /* Colossal Whale: low brass */
    {3,185,77,105,24,{0},0,0,0,78,60},  /* 169: impact into a sub drone */
    {54,185,102,96,2,{7,12},2,84,1,20,7}, /* Serpent: coiling intervals */
    {22,145,91,104,8,{12,19},2,44,3,0,26} /* Space Bloop: satellite chimes */
};
