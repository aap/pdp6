#include "pdp6.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <pthread.h>

SDL_Surface *screen;

SDL_Surface *keysurf[3];
SDL_Surface *lampsurf[2];
SDL_Surface *switchsurf[2];

typedef struct Key Key;
struct Key {
	SDL_Surface **surfs;
	SDL_Rect r;
	int state;
};

typedef struct Light Light;
struct Light {
	SDL_Surface **surfs;
	SDL_Rect r;
	int state;
};

typedef struct Switch Switch;
struct Switch {
	SDL_Surface **surfs;
	SDL_Rect r;
	int state;
	int active;	/* mouse down */
};

Key keys[] = {
	{ keysurf, {  646, 139, 18, 32 }, 0 },
	{ keysurf, {  714, 139, 18, 32 }, 0 },
	{ keysurf, {  782, 139, 18, 32 }, 0 },
	{ keysurf, {  850, 139, 18, 32 }, 0 },
	{ keysurf, {  918, 139, 18, 32 }, 0 },
	{ keysurf, {  985, 139, 18, 32 }, 0 },
	{ keysurf, { 1051, 139, 18, 32 }, 0 },
	{ keysurf, { 1117, 139, 18, 32 }, 0 },
};

Light op_lights[] = {
	/* IR */
	{ lampsurf, {  79, 43, 12, 12 }, 0 },
	{ lampsurf, {  93, 43, 12, 12 }, 0 },
	{ lampsurf, { 107, 43, 12, 12 }, 0 },
	{ lampsurf, { 122, 43, 12, 12 }, 0 },
	{ lampsurf, { 136, 43, 12, 12 }, 0 },
	{ lampsurf, { 150, 43, 12, 12 }, 0 },
	{ lampsurf, { 165, 43, 12, 12 }, 0 },
	{ lampsurf, { 179, 43, 12, 12 }, 0 },
	{ lampsurf, { 193, 43, 12, 12 }, 0 },
	{ lampsurf, { 208, 43, 12, 12 }, 0 },
	{ lampsurf, { 222, 43, 12, 12 }, 0 },
	{ lampsurf, { 236, 43, 12, 12 }, 0 },
	{ lampsurf, { 250, 43, 12, 12 }, 0 },
	{ lampsurf, { 265, 43, 12, 12 }, 0 },
	{ lampsurf, { 280, 43, 12, 12 }, 0 },
	{ lampsurf, { 294, 43, 12, 12 }, 0 },
	{ lampsurf, { 308, 43, 12, 12 }, 0 },
	{ lampsurf, { 322, 43, 12, 12 }, 0 },
	/* MI */
	{ lampsurf, {  79, 75, 12, 12 }, 0 },
	{ lampsurf, {  93, 75, 12, 12 }, 0 },
	{ lampsurf, { 107, 75, 12, 12 }, 0 },
	{ lampsurf, { 122, 75, 12, 12 }, 0 },
	{ lampsurf, { 136, 75, 12, 12 }, 0 },
	{ lampsurf, { 150, 75, 12, 12 }, 0 },
	{ lampsurf, { 165, 75, 12, 12 }, 0 },
	{ lampsurf, { 179, 75, 12, 12 }, 0 },
	{ lampsurf, { 193, 75, 12, 12 }, 0 },
	{ lampsurf, { 208, 75, 12, 12 }, 0 },
	{ lampsurf, { 222, 75, 12, 12 }, 0 },
	{ lampsurf, { 236, 75, 12, 12 }, 0 },
	{ lampsurf, { 251, 75, 12, 12 }, 0 },
	{ lampsurf, { 265, 75, 12, 12 }, 0 },
	{ lampsurf, { 279, 75, 12, 12 }, 0 },
	{ lampsurf, { 294, 75, 12, 12 }, 0 },
	{ lampsurf, { 308, 75, 12, 12 }, 0 },
	{ lampsurf, { 322, 75, 12, 12 }, 0 },
	{ lampsurf, { 337, 75, 12, 12 }, 0 },
	{ lampsurf, { 351, 75, 12, 12 }, 0 },
	{ lampsurf, { 365, 75, 12, 12 }, 0 },
	{ lampsurf, { 380, 75, 12, 12 }, 0 },
	{ lampsurf, { 394, 75, 12, 12 }, 0 },
	{ lampsurf, { 408, 75, 12, 12 }, 0 },
	{ lampsurf, { 423, 75, 12, 12 }, 0 },
	{ lampsurf, { 437, 75, 12, 12 }, 0 },
	{ lampsurf, { 451, 75, 12, 12 }, 0 },
	{ lampsurf, { 466, 75, 12, 12 }, 0 },
	{ lampsurf, { 480, 75, 12, 12 }, 0 },
	{ lampsurf, { 494, 75, 12, 12 }, 0 },
	{ lampsurf, { 509, 75, 12, 12 }, 0 },
	{ lampsurf, { 523, 75, 12, 12 }, 0 },
	{ lampsurf, { 537, 75, 12, 12 }, 0 },
	{ lampsurf, { 552, 75, 12, 12 }, 0 },
	{ lampsurf, { 566, 75, 12, 12 }, 0 },
	{ lampsurf, { 580, 75, 12, 12 }, 0 },
	/* PC */
	{ lampsurf, { 643, 43, 12, 12 }, 0 },
	{ lampsurf, { 657, 43, 12, 12 }, 0 },
	{ lampsurf, { 671, 43, 12, 12 }, 0 },
	{ lampsurf, { 686, 43, 12, 12 }, 0 },
	{ lampsurf, { 700, 43, 12, 12 }, 0 },
	{ lampsurf, { 714, 43, 12, 12 }, 0 },
	{ lampsurf, { 729, 43, 12, 12 }, 0 },
	{ lampsurf, { 743, 43, 12, 12 }, 0 },
	{ lampsurf, { 757, 43, 12, 12 }, 0 },
	{ lampsurf, { 772, 43, 12, 12 }, 0 },
	{ lampsurf, { 786, 43, 12, 12 }, 0 },
	{ lampsurf, { 800, 43, 12, 12 }, 0 },
	{ lampsurf, { 815, 43, 12, 12 }, 0 },
	{ lampsurf, { 829, 43, 12, 12 }, 0 },
	{ lampsurf, { 843, 43, 12, 12 }, 0 },
	{ lampsurf, { 857, 43, 12, 12 }, 0 },
	{ lampsurf, { 872, 43, 12, 12 }, 0 },
	{ lampsurf, { 886, 43, 12, 12 }, 0 },
	/* MA */
	{ lampsurf, { 643, 75, 12, 12 }, 0 },
	{ lampsurf, { 657, 75, 12, 12 }, 0 },
	{ lampsurf, { 671, 75, 12, 12 }, 0 },
	{ lampsurf, { 686, 75, 12, 12 }, 0 },
	{ lampsurf, { 700, 75, 12, 12 }, 0 },
	{ lampsurf, { 714, 75, 12, 12 }, 0 },
	{ lampsurf, { 729, 75, 12, 12 }, 0 },
	{ lampsurf, { 743, 75, 12, 12 }, 0 },
	{ lampsurf, { 757, 75, 12, 12 }, 0 },
	{ lampsurf, { 772, 75, 12, 12 }, 0 },
	{ lampsurf, { 786, 75, 12, 12 }, 0 },
	{ lampsurf, { 800, 75, 12, 12 }, 0 },
	{ lampsurf, { 815, 75, 12, 12 }, 0 },
	{ lampsurf, { 829, 75, 12, 12 }, 0 },
	{ lampsurf, { 843, 75, 12, 12 }, 0 },
	{ lampsurf, { 857, 75, 12, 12 }, 0 },
	{ lampsurf, { 872, 75, 12, 12 }, 0 },
	{ lampsurf, { 886, 75, 12, 12 }, 0 },
	/* PIO */
	{ lampsurf, {  974, 43, 12, 12 }, 0 },
	{ lampsurf, {  988, 43, 12, 12 }, 0 },
	{ lampsurf, { 1002, 43, 12, 12 }, 0 },
	{ lampsurf, { 1016, 43, 12, 12 }, 0 },
	{ lampsurf, { 1030, 43, 12, 12 }, 0 },
	{ lampsurf, { 1044, 43, 12, 12 }, 0 },
	{ lampsurf, { 1058, 43, 12, 12 }, 0 },
	/* PIR */
	{ lampsurf, {  974, 75, 12, 12 }, 0 },
	{ lampsurf, {  988, 75, 12, 12 }, 0 },
	{ lampsurf, { 1002, 75, 12, 12 }, 0 },
	{ lampsurf, { 1016, 75, 12, 12 }, 0 },
	{ lampsurf, { 1030, 75, 12, 12 }, 0 },
	{ lampsurf, { 1044, 75, 12, 12 }, 0 },
	{ lampsurf, { 1058, 75, 12, 12 }, 0 },
	/* PIH */
	{ lampsurf, {  974, 107, 12, 12 }, 0 },
	{ lampsurf, {  988, 107, 12, 12 }, 0 },
	{ lampsurf, { 1002, 107, 12, 12 }, 0 },
	{ lampsurf, { 1016, 107, 12, 12 }, 0 },
	{ lampsurf, { 1030, 107, 12, 12 }, 0 },
	{ lampsurf, { 1044, 107, 12, 12 }, 0 },
	{ lampsurf, { 1058, 107, 12, 12 }, 0 },
	/* Address stop, Repeat */
	{ lampsurf, { 1126,  75, 12, 12 }, 0 },
	{ lampsurf, { 1126, 107, 12, 12 }, 0 },
	/* Disable memory, Power */
	{ lampsurf, { 1194,  75, 12, 12 }, 0 },
	{ lampsurf, { 1194, 107, 12, 12 }, 0 },
	/* Run, Mem stop, PI on */
	{ lampsurf, { 946,  43, 12, 12 }, 0 },
	{ lampsurf, { 946,  75, 12, 12 }, 0 },
	{ lampsurf, { 946, 107, 12, 12 }, 0 },
};

Light ind_lights[] = {
	/* MB */
	{ lampsurf, {  713, 74, 12, 12 }, 0 },
	{ lampsurf, {  727, 74, 12, 12 }, 0 },
	{ lampsurf, {  741, 74, 12, 12 }, 0 },
	{ lampsurf, {  756, 74, 12, 12 }, 0 },
	{ lampsurf, {  770, 74, 12, 12 }, 0 },
	{ lampsurf, {  784, 74, 12, 12 }, 0 },
	{ lampsurf, {  799, 74, 12, 12 }, 0 },
	{ lampsurf, {  813, 74, 12, 12 }, 0 },
	{ lampsurf, {  827, 74, 12, 12 }, 0 },
	{ lampsurf, {  842, 74, 12, 12 }, 0 },
	{ lampsurf, {  856, 74, 12, 12 }, 0 },
	{ lampsurf, {  870, 74, 12, 12 }, 0 },
	{ lampsurf, {  885, 74, 12, 12 }, 0 },
	{ lampsurf, {  899, 74, 12, 12 }, 0 },
	{ lampsurf, {  913, 74, 12, 12 }, 0 },
	{ lampsurf, {  928, 74, 12, 12 }, 0 },
	{ lampsurf, {  942, 74, 12, 12 }, 0 },
	{ lampsurf, {  956, 74, 12, 12 }, 0 },
	{ lampsurf, {  971, 74, 12, 12 }, 0 },
	{ lampsurf, {  985, 74, 12, 12 }, 0 },
	{ lampsurf, {  999, 74, 12, 12 }, 0 },
	{ lampsurf, { 1014, 74, 12, 12 }, 0 },
	{ lampsurf, { 1028, 74, 12, 12 }, 0 },
	{ lampsurf, { 1042, 74, 12, 12 }, 0 },
	{ lampsurf, { 1057, 74, 12, 12 }, 0 },
	{ lampsurf, { 1071, 74, 12, 12 }, 0 },
	{ lampsurf, { 1085, 74, 12, 12 }, 0 },
	{ lampsurf, { 1100, 74, 12, 12 }, 0 },
	{ lampsurf, { 1114, 74, 12, 12 }, 0 },
	{ lampsurf, { 1128, 74, 12, 12 }, 0 },
	{ lampsurf, { 1143, 74, 12, 12 }, 0 },
	{ lampsurf, { 1157, 74, 12, 12 }, 0 },
	{ lampsurf, { 1171, 74, 12, 12 }, 0 },
	{ lampsurf, { 1186, 74, 12, 12 }, 0 },
	{ lampsurf, { 1200, 74, 12, 12 }, 0 },
	{ lampsurf, { 1214, 74, 12, 12 }, 0 },
	/* AR */
	{ lampsurf, {  713, 110, 12, 12 }, 0 },
	{ lampsurf, {  727, 110, 12, 12 }, 0 },
	{ lampsurf, {  741, 110, 12, 12 }, 0 },
	{ lampsurf, {  756, 110, 12, 12 }, 0 },
	{ lampsurf, {  770, 110, 12, 12 }, 0 },
	{ lampsurf, {  784, 110, 12, 12 }, 0 },
	{ lampsurf, {  799, 110, 12, 12 }, 0 },
	{ lampsurf, {  813, 110, 12, 12 }, 0 },
	{ lampsurf, {  827, 110, 12, 12 }, 0 },
	{ lampsurf, {  842, 110, 12, 12 }, 0 },
	{ lampsurf, {  856, 110, 12, 12 }, 0 },
	{ lampsurf, {  870, 110, 12, 12 }, 0 },
	{ lampsurf, {  885, 110, 12, 12 }, 0 },
	{ lampsurf, {  899, 110, 12, 12 }, 0 },
	{ lampsurf, {  913, 110, 12, 12 }, 0 },
	{ lampsurf, {  928, 110, 12, 12 }, 0 },
	{ lampsurf, {  942, 110, 12, 12 }, 0 },
	{ lampsurf, {  956, 110, 12, 12 }, 0 },
	{ lampsurf, {  971, 110, 12, 12 }, 0 },
	{ lampsurf, {  985, 110, 12, 12 }, 0 },
	{ lampsurf, {  999, 110, 12, 12 }, 0 },
	{ lampsurf, { 1014, 110, 12, 12 }, 0 },
	{ lampsurf, { 1028, 110, 12, 12 }, 0 },
	{ lampsurf, { 1042, 110, 12, 12 }, 0 },
	{ lampsurf, { 1057, 110, 12, 12 }, 0 },
	{ lampsurf, { 1071, 110, 12, 12 }, 0 },
	{ lampsurf, { 1085, 110, 12, 12 }, 0 },
	{ lampsurf, { 1100, 110, 12, 12 }, 0 },
	{ lampsurf, { 1114, 110, 12, 12 }, 0 },
	{ lampsurf, { 1128, 110, 12, 12 }, 0 },
	{ lampsurf, { 1143, 110, 12, 12 }, 0 },
	{ lampsurf, { 1157, 110, 12, 12 }, 0 },
	{ lampsurf, { 1171, 110, 12, 12 }, 0 },
	{ lampsurf, { 1186, 110, 12, 12 }, 0 },
	{ lampsurf, { 1200, 110, 12, 12 }, 0 },
	{ lampsurf, { 1214, 110, 12, 12 }, 0 },
	/* MQ */
	{ lampsurf, {  713, 146, 12, 12 }, 0 },
	{ lampsurf, {  727, 146, 12, 12 }, 0 },
	{ lampsurf, {  741, 146, 12, 12 }, 0 },
	{ lampsurf, {  756, 146, 12, 12 }, 0 },
	{ lampsurf, {  770, 146, 12, 12 }, 0 },
	{ lampsurf, {  784, 146, 12, 12 }, 0 },
	{ lampsurf, {  799, 146, 12, 12 }, 0 },
	{ lampsurf, {  813, 146, 12, 12 }, 0 },
	{ lampsurf, {  827, 146, 12, 12 }, 0 },
	{ lampsurf, {  842, 146, 12, 12 }, 0 },
	{ lampsurf, {  856, 146, 12, 12 }, 0 },
	{ lampsurf, {  870, 146, 12, 12 }, 0 },
	{ lampsurf, {  885, 146, 12, 12 }, 0 },
	{ lampsurf, {  899, 146, 12, 12 }, 0 },
	{ lampsurf, {  913, 146, 12, 12 }, 0 },
	{ lampsurf, {  928, 146, 12, 12 }, 0 },
	{ lampsurf, {  942, 146, 12, 12 }, 0 },
	{ lampsurf, {  956, 146, 12, 12 }, 0 },
	{ lampsurf, {  971, 146, 12, 12 }, 0 },
	{ lampsurf, {  985, 146, 12, 12 }, 0 },
	{ lampsurf, {  999, 146, 12, 12 }, 0 },
	{ lampsurf, { 1014, 146, 12, 12 }, 0 },
	{ lampsurf, { 1028, 146, 12, 12 }, 0 },
	{ lampsurf, { 1042, 146, 12, 12 }, 0 },
	{ lampsurf, { 1057, 146, 12, 12 }, 0 },
	{ lampsurf, { 1071, 146, 12, 12 }, 0 },
	{ lampsurf, { 1085, 146, 12, 12 }, 0 },
	{ lampsurf, { 1100, 146, 12, 12 }, 0 },
	{ lampsurf, { 1114, 146, 12, 12 }, 0 },
	{ lampsurf, { 1128, 146, 12, 12 }, 0 },
	{ lampsurf, { 1143, 146, 12, 12 }, 0 },
	{ lampsurf, { 1157, 146, 12, 12 }, 0 },
	{ lampsurf, { 1171, 146, 12, 12 }, 0 },
	{ lampsurf, { 1186, 146, 12, 12 }, 0 },
	{ lampsurf, { 1200, 146, 12, 12 }, 0 },
	{ lampsurf, { 1214, 146, 12, 12 }, 0 },
	/* flags */
	/* FE */
	{ lampsurf, { 539, 161, 14, 22 }, 0 },
	{ lampsurf, { 451,  42, 14, 22 }, 0 },
	{ lampsurf, { 451,  59, 14, 22 }, 0 },
	{ lampsurf, { 451,  76, 14, 22 }, 0 },
	{ lampsurf, { 451,  93, 14, 22 }, 0 },
	{ lampsurf, { 451, 110, 14, 22 }, 0 },
	{ lampsurf, { 451, 127, 14, 22 }, 0 },
	{ lampsurf, { 451, 144, 14, 22 }, 0 },
	{ lampsurf, { 451, 161, 14, 22 }, 0 },
	/* SC */
	{ lampsurf, { 539, 144, 14, 22 }, 0 },
	{ lampsurf, { 495,  42, 14, 22 }, 0 },
	{ lampsurf, { 495,  59, 14, 22 }, 0 },
	{ lampsurf, { 495,  76, 14, 22 }, 0 },
	{ lampsurf, { 495,  93, 14, 22 }, 0 },
	{ lampsurf, { 495, 110, 14, 22 }, 0 },
	{ lampsurf, { 495, 127, 14, 22 }, 0 },
	{ lampsurf, { 495, 144, 14, 22 }, 0 },
	{ lampsurf, { 495, 161, 14, 22 }, 0 },
	/* misc flip-flops */
	/* column 1 */
	{ lampsurf, {  55,  42, 14, 22 }, 0 },
	{ lampsurf, {  55,  59, 14, 22 }, 0 },
	{ lampsurf, {  55,  76, 14, 22 }, 0 },
	{ lampsurf, {  55,  93, 14, 22 }, 0 },
	{ lampsurf, {  55, 110, 14, 22 }, 0 },
	{ lampsurf, {  55, 127, 14, 22 }, 0 },
	{ lampsurf, {  55, 144, 14, 22 }, 0 },
	{ lampsurf, {  55, 161, 14, 22 }, 0 },
	/* */
	{ lampsurf, { 319, 110, 14, 22 }, 0 },
	{ lampsurf, { 319, 127, 14, 22 }, 0 },
	/* column 12 */
	{ lampsurf, { 539,  42, 14, 22 }, 0 },
	{ lampsurf, { 539,  59, 14, 22 }, 0 },
	{ lampsurf, { 539,  76, 14, 22 }, 0 },
	{ lampsurf, { 539,  93, 14, 22 }, 0 },
	{ lampsurf, { 539, 110, 14, 22 }, 0 },
	{ lampsurf, { 539, 127, 14, 22 }, 0 },
	/* column 13 */
	{ lampsurf, { 583,  42, 14, 22 }, 0 },
	{ lampsurf, { 583,  59, 14, 22 }, 0 },
	{ lampsurf, { 583,  76, 14, 22 }, 0 },
	{ lampsurf, { 583,  93, 14, 22 }, 0 },
	{ lampsurf, { 583, 110, 14, 22 }, 0 },
	{ lampsurf, { 583, 127, 14, 22 }, 0 },
	{ lampsurf, { 583, 144, 14, 22 }, 0 },
	{ lampsurf, { 583, 161, 14, 22 }, 0 },
	/* column 14 */
	{ lampsurf, { 627,  42, 14, 22 }, 0 },
	{ lampsurf, { 627,  59, 14, 22 }, 0 },
	{ lampsurf, { 627,  76, 14, 22 }, 0 },
	{ lampsurf, { 627,  93, 14, 22 }, 0 },
	{ lampsurf, { 627, 110, 14, 22 }, 0 },
	{ lampsurf, { 627, 127, 14, 22 }, 0 },
	{ lampsurf, { 627, 144, 14, 22 }, 0 },
	{ lampsurf, { 627, 161, 14, 22 }, 0 },
	/* column 9 */
	{ lampsurf, { 407,  42, 14, 22 }, 0 },
	{ lampsurf, { 407,  59, 14, 22 }, 0 },
	{ lampsurf, { 407,  76, 14, 22 }, 0 },
	{ lampsurf, { 407,  93, 14, 22 }, 0 },
	{ lampsurf, { 407, 110, 14, 22 }, 0 },
	{ lampsurf, { 407, 127, 14, 22 }, 0 },
	{ lampsurf, { 407, 144, 14, 22 }, 0 },
	{ lampsurf, { 407, 161, 14, 22 }, 0 },
	/* sbr flip-flops */
	{ lampsurf, {  99,  42, 14, 22 }, 0 },
	{ lampsurf, {  99,  59, 14, 22 }, 0 },
	{ lampsurf, {  99,  76, 14, 22 }, 0 },
	{ lampsurf, {  99,  93, 14, 22 }, 0 },
	{ lampsurf, {  99, 110, 14, 22 }, 0 },
	{ lampsurf, {  99, 127, 14, 22 }, 0 },
	{ lampsurf, {  99, 144, 14, 22 }, 0 },
	{ lampsurf, {  99, 161, 14, 22 }, 0 },
};

Light extra_lights[] = {
	/* MEMBUS */
	{ lampsurf, {  693, 26, 12, 12 }, 0 },
	{ lampsurf, {  707, 26, 12, 12 }, 0 },
	{ lampsurf, {  721, 26, 12, 12 }, 0 },
	{ lampsurf, {  736, 26, 12, 12 }, 0 },
	{ lampsurf, {  750, 26, 12, 12 }, 0 },
	{ lampsurf, {  764, 26, 12, 12 }, 0 },
	{ lampsurf, {  779, 26, 12, 12 }, 0 },
	{ lampsurf, {  793, 26, 12, 12 }, 0 },
	{ lampsurf, {  807, 26, 12, 12 }, 0 },
	{ lampsurf, {  822, 26, 12, 12 }, 0 },
	{ lampsurf, {  836, 26, 12, 12 }, 0 },
	{ lampsurf, {  850, 26, 12, 12 }, 0 },
	{ lampsurf, {  865, 26, 12, 12 }, 0 },
	{ lampsurf, {  879, 26, 12, 12 }, 0 },
	{ lampsurf, {  893, 26, 12, 12 }, 0 },
	{ lampsurf, {  908, 26, 12, 12 }, 0 },
	{ lampsurf, {  922, 26, 12, 12 }, 0 },
	{ lampsurf, {  936, 26, 12, 12 }, 0 },
	{ lampsurf, {  951, 26, 12, 12 }, 0 },
	{ lampsurf, {  965, 26, 12, 12 }, 0 },
	{ lampsurf, {  979, 26, 12, 12 }, 0 },
	{ lampsurf, {  994, 26, 12, 12 }, 0 },
	{ lampsurf, { 1008, 26, 12, 12 }, 0 },
	{ lampsurf, { 1022, 26, 12, 12 }, 0 },
	{ lampsurf, { 1037, 26, 12, 12 }, 0 },
	{ lampsurf, { 1051, 26, 12, 12 }, 0 },
	{ lampsurf, { 1065, 26, 12, 12 }, 0 },
	{ lampsurf, { 1080, 26, 12, 12 }, 0 },
	{ lampsurf, { 1094, 26, 12, 12 }, 0 },
	{ lampsurf, { 1108, 26, 12, 12 }, 0 },
	{ lampsurf, { 1123, 26, 12, 12 }, 0 },
	{ lampsurf, { 1137, 26, 12, 12 }, 0 },
	{ lampsurf, { 1151, 26, 12, 12 }, 0 },
	{ lampsurf, { 1166, 26, 12, 12 }, 0 },
	{ lampsurf, { 1180, 26, 12, 12 }, 0 },
	{ lampsurf, { 1194, 26, 12, 12 }, 0 },
	/* PR */
	{ lampsurf, {   74, 26, 12, 12 }, 0 },
	{ lampsurf, {   88, 26, 12, 12 }, 0 },
	{ lampsurf, {  102, 26, 12, 12 }, 0 },
	{ lampsurf, {  117, 26, 12, 12 }, 0 },
	{ lampsurf, {  131, 26, 12, 12 }, 0 },
	{ lampsurf, {  145, 26, 12, 12 }, 0 },
	{ lampsurf, {  160, 26, 12, 12 }, 0 },
	{ lampsurf, {  174, 26, 12, 12 }, 0 },
	/* RLR */
	{ lampsurf, {  274, 26, 12, 12 }, 0 },
	{ lampsurf, {  288, 26, 12, 12 }, 0 },
	{ lampsurf, {  302, 26, 12, 12 }, 0 },
	{ lampsurf, {  317, 26, 12, 12 }, 0 },
	{ lampsurf, {  331, 26, 12, 12 }, 0 },
	{ lampsurf, {  345, 26, 12, 12 }, 0 },
	{ lampsurf, {  360, 26, 12, 12 }, 0 },
	{ lampsurf, {  374, 26, 12, 12 }, 0 },
	/* RLA */
	{ lampsurf, {  475, 26, 12, 12 }, 0 },
	{ lampsurf, {  489, 26, 12, 12 }, 0 },
	{ lampsurf, {  503, 26, 12, 12 }, 0 },
	{ lampsurf, {  518, 26, 12, 12 }, 0 },
	{ lampsurf, {  532, 26, 12, 12 }, 0 },
	{ lampsurf, {  546, 26, 12, 12 }, 0 },
	{ lampsurf, {  561, 26, 12, 12 }, 0 },
	{ lampsurf, {  575, 26, 12, 12 }, 0 },
};
Light *ir_lght, *mi_lght, *pc_lght, *ma_lght, *pio_lght, *pir_lght,
	*pih_lght, *rest_lght;
Light *mb_lght, *pc_lght, *ar_lght, *mq_lght, *fe_lght, *sc_lght,
	*ff_lght;
Light *membus_lght, *pr_lght, *rlr_lght, *rla_lght;

Switch switches[] = {
	/* DATA */
	{ switchsurf, {  78, 102, 14, 22 }, 0, 0 },
	{ switchsurf, {  92, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 106, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 121, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 135, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 149, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 164, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 178, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 192, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 207, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 221, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 235, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 250, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 264, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 278, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 293, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 307, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 321, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 336, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 350, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 364, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 379, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 393, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 407, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 422, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 436, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 450, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 465, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 479, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 493, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 508, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 522, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 536, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 551, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 565, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 579, 102, 14, 22 }, 0, 0 },
	/* MAS */
	{ switchsurf, { 642, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 656, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 670, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 685, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 699, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 713, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 728, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 742, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 756, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 771, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 785, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 799, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 814, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 828, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 842, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 857, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 871, 102, 14, 22 }, 0, 0 },
	{ switchsurf, { 885, 102, 14, 22 }, 0, 0 },
	/* Address stop, Repeat */
	{ switchsurf, { 1111,  70, 14, 22 }, 0, 0 },
	{ switchsurf, { 1111, 102, 14, 22 }, 0, 0 },
	/* Disable memory, Power */
	{ switchsurf, { 1179,  70, 14, 22 }, 0, 0 },
	{ switchsurf, { 1179, 102, 14, 22 }, 0, 0 },

	/* RIM MAINT */
	{ switchsurf, { 633, 21, 14, 22 }, 0, 0 },
};
Switch *data_sw, *ma_sw, *rest_sw, *rim_maint_sw;

void
setlights(word w, Light *l, int n)
{
	int i;
	for(i = 0; i < n; i++)
		l[n-i-1].state = !!(w & 1L<<i);
}

word
getswitches(Switch *sw, int n)
{
	word w;
	int i;
	w = 0;
	for(i = 0; i < n; i++)
		w |= (word)sw[n-i-1].state << i;
	return w;
}

void
poweron(void)
{
	pthread_t apr_thread;
	apr.sw_power = 1;
	pthread_create(&apr_thread, NULL, aprmain, NULL);
}

void
mouse(int button, int state, int x, int y)
{
	static int buttonstate;
	int prevst;
	int i;
	SDL_Rect *r;

	if(button){
		if(state == 1)
			buttonstate |= 1<<button-1;
		else
			buttonstate &= ~(1<<button-1);
	}

	for(i = 0; i < nelem(switches); i++){
		r = &switches[i].r;
		if(buttonstate == 0 ||
		   x < r->x || x > r->x+r->w ||
		   y < r->y || y > r->y+r->h){
			switches[i].active = 0;
			continue;
		}
		if(!switches[i].active){
			prevst = switches[i].state;
			if(buttonstate & 1)
				switches[i].state = !switches[i].state;
			if(buttonstate & 2)
				switches[i].state = 1;
			if(buttonstate & 4)
				switches[i].state = 0;
			switches[i].active = 1;

			/* state changed */
			if(prevst != switches[i].state){
				/* power */
				if(&switches[i] == &rest_sw[3]){
					if(prevst == 0)
						poweron();
				}
				/* rim maint */
				if(&switches[i] == rim_maint_sw){
					if(prevst == 0)
						apr.key_rim_sbr = 1;
				}
			}
		}
	}

	for(i = 0; i < nelem(keys); i++){
		r = &keys[i].r;
		if(buttonstate == 0 ||
		   x < r->x || x > r->x+r->w ||
		   y < r->y || y > r->y+r->h){
			keys[i].state = 0;
			continue;
		}
		prevst = keys[i].state;
		if(buttonstate & 1)
			keys[i].state = 1;
		if(buttonstate & 4)
			keys[i].state = 2;
		if(prevst != keys[i].state){
			switch(i){
			case 0:	/* start */
			case 1:	/* cont */
			case 3:	/* execute, reset */
			case 4:	/* deposit */
			case 5:	/* examine */
				if(keys[i].state && apr.sw_power)
					apr.extpulse |= 1;
				break;
			case 2:	/* stop */
			case 6:	/* on off reader */
			case 7: /* punch */
				break;
			}
		}
	}
}

int
main()
{
	SDL_Event ev;
	SDL_MouseButtonEvent *mbev;
	SDL_MouseMotionEvent *mmev;
	SDL_Surface *op_surf, *ind_surf, *extra_surf;
	SDL_Rect op_panel = { 0, 274, 1280, 210 };
	SDL_Rect ind_panel = { 0, 64, 1280, 210 };
	SDL_Rect extra_panel = { 0, 0, 1280, 210 };
	int i;
	Light *l;
	Switch *sw;

	if(SDL_Init(SDL_INIT_VIDEO) < 0){
error:
		fprintf(stderr, "error: %s\n", SDL_GetError());
		return 1;
	}
	screen = SDL_SetVideoMode(1280, 484, 32, SDL_DOUBLEBUF);
	if(screen == NULL)
		goto error;

	if((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG){
		fprintf(stderr, "error: init SDL_Image: %s\n", IMG_GetError());
		return 1;
	}

	op_surf = IMG_Load("op_panel.png");
	if(op_surf == NULL){
		fprintf(stderr, "Couldn't load op_panel.png\n");
		return 1;
	}
	ind_surf = IMG_Load("ind_panel.png");
	if(ind_surf == NULL){
		fprintf(stderr, "Couldn't load ind_panel.png\n");
		return 1;
	}
	extra_surf = IMG_Load("extra_panel.png");
	if(extra_surf == NULL){
		fprintf(stderr, "Couldn't load extra_panel.png\n");
		return 1;
	}

	keysurf[0] = IMG_Load("key_n.png");
	if(keysurf[0] == NULL){
		fprintf(stderr, "Couldn't load key_n.png\n");
		return 1;
	}
	keysurf[1] = IMG_Load("key_d.png");
	if(keysurf[1] == NULL){
		fprintf(stderr, "Couldn't load key_d.png\n");
		return 1;
	}
	keysurf[2] = IMG_Load("key_u.png");
	if(keysurf[2] == NULL){
		fprintf(stderr, "Couldn't load key_u.png\n");
		return 1;
	}

	lampsurf[0] = IMG_Load("lamp_off.png");
	if(lampsurf[0] == NULL){
		fprintf(stderr, "Couldn't load lamp_off.png\n");
		return 1;
	}
	lampsurf[1] = IMG_Load("lamp_on.png");
	if(lampsurf[1] == NULL){
		fprintf(stderr, "Couldn't load lamp_on.png\n");
		return 1;
	}

	switchsurf[0] = IMG_Load("switch_d.png");
	if(switchsurf[0] == NULL){
		fprintf(stderr, "Couldn't load switch_d.png\n");
		return 1;
	}
	switchsurf[1] = IMG_Load("switch_u.png");
	if(switchsurf[1] == NULL){
		fprintf(stderr, "Couldn't load switch_u.png\n");
		return 1;
	}

	l = op_lights;
	ir_lght   = l; l += 18;
	mi_lght   = l; l += 36;
	pc_lght   = l; l += 18;
	ma_lght   = l; l += 18;
	pio_lght  = l; l += 7;
	pir_lght  = l; l += 7;
	pih_lght  = l; l += 7;
	rest_lght = l;
	sw = switches;
	data_sw      = sw; sw += 36;
	ma_sw        = sw; sw += 18;
	rest_sw      = sw; sw += 4;
	rim_maint_sw = sw;
	l = ind_lights;
	mb_lght   = l; l += 36;
	ar_lght   = l; l += 36;
	mq_lght   = l; l += 36;
	fe_lght   = l; l += 9;
	sc_lght   = l; l += 9;
	ff_lght   = l;
	l = extra_lights;
	membus_lght = l; l += 36;
	pr_lght     = l; l += 8;
	rlr_lght    = l; l += 8;
	rla_lght    = l;
	for(i = 0; i < nelem(keys); i++){
		keys[i].r.x += op_panel.x;
		keys[i].r.y += op_panel.y;
	}
	for(i = 0; i < nelem(op_lights); i++){
		op_lights[i].r.x += op_panel.x;
		op_lights[i].r.y += op_panel.y;
	}
	for(i = 0; i < nelem(ind_lights); i++){
		ind_lights[i].r.x += ind_panel.x;
		ind_lights[i].r.y += ind_panel.y;
	}
	for(i = 0; i < nelem(extra_lights); i++){
		extra_lights[i].r.x += extra_panel.x;
		extra_lights[i].r.y += extra_panel.y;
	}
	for(i = 0; i < nelem(switches)-1; i++){
		switches[i].r.x += op_panel.x;
		switches[i].r.y += op_panel.y;
	}
	rim_maint_sw->r.x += extra_panel.x;
	rim_maint_sw->r.y += extra_panel.y;

	initmem();
	memset(&apr, 0xff, sizeof apr);
	apr.extpulse = 0;
	apr.nextpulse = apr.mc_rst1_ret = apr.art3_ret = NULL;

	for(;;){
		while(SDL_PollEvent(&ev))
			switch(ev.type){
			case SDL_MOUSEMOTION:
				mmev = (SDL_MouseMotionEvent*)&ev;
				mouse(0, mmev->state,
				      mmev->x, mmev->y);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mbev = (SDL_MouseButtonEvent*)&ev;
				mouse(mbev->button, mbev->state,
				      mbev->x, mbev->y);
				break;
			case SDL_QUIT:
				SDL_Quit();
				return 0;
			}
		setlights(apr.ir, ir_lght, 18);
		setlights(apr.mi, mi_lght, 36);
		setlights(apr.pc, pc_lght, 18);
		setlights(apr.ma, ma_lght, 18);
		setlights(apr.pio, pio_lght, 7);
		setlights(apr.pir, pir_lght, 7);
		setlights(apr.pih, pih_lght, 7);
		rest_lght[4].state = apr.run;
		rest_lght[5].state = apr.mc_stop;
		rest_lght[6].state = apr.pi_active;
		rest_lght[0].state = apr.sw_addr_stop   = rest_sw[0].state;
		rest_lght[1].state = apr.sw_repeat      = rest_sw[1].state;
		rest_lght[2].state = apr.sw_mem_disable = rest_sw[2].state;
		rest_lght[3].state = apr.sw_power       = rest_sw[3].state;
		apr.sw_rim_maint = rim_maint_sw->state;
		apr.data = getswitches(data_sw, 36);
		apr.mas = getswitches(ma_sw, 18);
		apr.key_start     = keys[0].state == 1;
		apr.key_readin    = keys[0].state == 2;
		apr.key_inst_cont = keys[1].state == 1;
		apr.key_mem_cont  = keys[1].state == 2;
		apr.key_inst_stop = keys[2].state == 1;
		apr.key_mem_stop  = keys[2].state == 2;
		apr.key_io_reset  = keys[3].state == 1;
		apr.key_execute   = keys[3].state == 2;
		apr.key_dep       = keys[4].state == 1;
		apr.key_dep_next  = keys[4].state == 2;
		apr.key_ex        = keys[5].state == 1;
		apr.key_ex_next   = keys[5].state == 2;
		apr.key_rd_off    = keys[6].state == 1;
		apr.key_rd_on     = keys[6].state == 2;
		apr.key_pt_rd     = keys[7].state == 1;
		apr.key_pt_wr     = keys[7].state == 2;

		setlights(apr.mb, mb_lght, 36);
		setlights(apr.ar, ar_lght, 36);
		setlights(apr.mq, mq_lght, 36);
		setlights(apr.fe, fe_lght, 9);
		setlights(apr.sc, sc_lght, 9);
		ff_lght[0].state = apr.key_ex_st;
		ff_lght[1].state = apr.key_ex_sync;
		ff_lght[2].state = apr.key_dep_st;
		ff_lght[3].state = apr.key_dep_sync;
		ff_lght[4].state = apr.key_rd_wr;
		ff_lght[5].state = apr.mc_rd;
		ff_lght[6].state = apr.mc_wr;
		ff_lght[7].state = apr.mc_rq;
		ff_lght[8].state = apr.mc_split_cyc_sync;
		ff_lght[9].state = apr.mc_stop_sync;
		ff_lght[10].state = !apr.ex_user;
		ff_lght[11].state = apr.cpa_illeg_op;
		ff_lght[12].state = apr.ex_ill_op;
		ff_lght[13].state = apr.ex_uuo_sync;
		ff_lght[14].state = apr.ex_pi_sync;
		ff_lght[15].state = apr.mq36;
		ff_lght[16].state = apr.key_rim_sbr;
		ff_lght[17].state = apr.cry0_cry1;
		ff_lght[18].state = apr.ar_cry0;
		ff_lght[19].state = apr.ar_cry1;
		ff_lght[20].state = apr.ar_ov_flag;
		ff_lght[21].state = apr.ar_cry0_flag;
		ff_lght[22].state = apr.ar_cry1_flag;
		ff_lght[23].state = apr.pc_chg_flag;
		ff_lght[24].state = apr.cpa_non_exist_mem;
		ff_lght[25].state = apr.cpa_clock_en;
		ff_lght[26].state = apr.cpa_clock_flag;
		ff_lght[27].state = apr.cpa_pc_chg_en;
		ff_lght[28].state = apr.cpa_arov_en;
		ff_lght[29].state = apr.cpa_pia33;
		ff_lght[30].state = apr.cpa_pia34;
		ff_lght[31].state = apr.cpa_pia35;
		ff_lght[32].state = apr.pi_ov;
		ff_lght[33].state = apr.pi_cyc;
		ff_lght[34].state = !!apr.pi_req;
		ff_lght[35].state = apr.iot_go;
		ff_lght[36].state = apr.a_long;
		ff_lght[37].state = apr.ma == apr.mas;
		ff_lght[38].state = apr.uuo_f1;
		ff_lght[39].state = apr.cpa_pdl_ov;
		ff_lght[40].state = apr.if1a;
		ff_lght[41].state = apr.af0;
		ff_lght[42].state = apr.af3;
		ff_lght[43].state = apr.af3a;
		ff_lght[44].state = apr.et4_ar_pse;
		ff_lght[45].state = apr.f1a;
		ff_lght[46].state = apr.f4a;
		ff_lght[47].state = apr.f6a;

		setlights(membus0, membus_lght, 36);
		setlights(apr.pr, pr_lght, 8);
		setlights(apr.rlr, rlr_lght, 8);
		setlights(apr.rla, rla_lght, 8);

		SDL_BlitSurface(op_surf, NULL, screen, &op_panel);
		SDL_BlitSurface(ind_surf, NULL, screen, &ind_panel);
		SDL_BlitSurface(extra_surf, NULL, screen, &extra_panel);
		for(i = 0; i < nelem(keys); i++)
			SDL_BlitSurface(keys[i].surfs[keys[i].state],
			                NULL, screen, &keys[i].r);
		for(i = 0; i < nelem(op_lights); i++)
			SDL_BlitSurface(op_lights[i].surfs[op_lights[i].state && apr.sw_power],
			                NULL, screen, &op_lights[i].r);
		for(i = 0; i < nelem(ind_lights); i++)
			SDL_BlitSurface(ind_lights[i].surfs[ind_lights[i].state && apr.sw_power],
			                NULL, screen, &ind_lights[i].r);
		for(i = 0; i < nelem(extra_lights); i++)
			SDL_BlitSurface(extra_lights[i].surfs[extra_lights[i].state && apr.sw_power],
			                NULL, screen, &extra_lights[i].r);
		for(i = 0; i < nelem(switches); i++)
			SDL_BlitSurface(switches[i].surfs[switches[i].state],
			                NULL, screen, &switches[i].r);
		SDL_Flip(screen);
	}
}
