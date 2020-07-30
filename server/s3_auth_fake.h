#pragma once

#ifndef __S3_SERVER_S3_AUTH_FAKE_H__
#define __S3_SERVER_S3_AUTH_FAKE_H__

int s3_auth_fake_evhtp_request(S3AuthClientOpType auth_request_type,
                               S3AuthClientOpContext *context);

#endif /* __S3_SERVER_S3_AUTH_FAKE_H__ */
