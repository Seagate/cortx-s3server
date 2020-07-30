#pragma once

#ifndef __S3_SERVER_S3_PERF_METRICS_H__

#include "event_utils.h"

int s3_perf_metrics_init(evbase_t *evbase);
void s3_perf_metrics_fini();

void s3_perf_count_incoming_bytes(int byte_count);
void s3_perf_count_outcoming_bytes(int byte_count);

#endif
