
#ifndef __VERIFICATION_H__
#define __VERIFICATION_H__

#include "xcl2.hpp"
#include "host_data_types.h"
#include "host_config.h"
#include "graph.h"


int verifyResults(partition_container_dt &partition_container, acc_descriptor_dt &acc, CSR* csr);
int verifyResultsXrt(partition_container_dt &partition_container, CSR* csr, const uint *device_prop);

#endif


