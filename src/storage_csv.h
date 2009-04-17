#ifndef __storage_csv_h__
#define __storage_csv_h__

#include <stdint.h>
#include "logger.h"
#include "util.h"
#include "dataparser.h"
#include "sflowparser.h"

void storage_csv_load();
void storage_csv_store_cntr(counter_list_t* list, uint32_t timestamp);

#endif
