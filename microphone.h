#ifndef SOUND_DETECTOR_H
#define SOUND_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>

//#define SOUND_THRESHOLD 1000

void sound_detector_init(void);
bool sound_detector_check(void);
int32_t sound_detector_get_level(void);

#endif