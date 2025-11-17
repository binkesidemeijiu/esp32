#ifndef __TFCARD_H_
#define __TFCARD_H_
#include "init_common.h"

#define TF_MISO  40
#define TF_MOSI  38
#define TF_CLK   39
#define TF_CS    1

void uget_sd_data(uint8_t *buf, uint32_t len);
#endif
