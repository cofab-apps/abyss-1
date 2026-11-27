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
