
#include "s3_common.h"

std::map<std::string, S3OperationCode, compare> S3OperationString{
    {"none", S3OperationCode::none}, {"acl", S3OperationCode::acl},
    {"encryption", S3OperationCode::encryption},
    {"location", S3OperationCode::location},
    {"policy", S3OperationCode::policy}, {"logging", S3OperationCode::logging},
    {"lifecycle", S3OperationCode::lifecycle}, {"cors", S3OperationCode::cors},
    {"notification", S3OperationCode::notification},
    {"replicaton", S3OperationCode::replicaton},
    {"tagging", S3OperationCode::tagging},
    {"requestPayment", S3OperationCode::requestPayment},
    {"versioning", S3OperationCode::versioning},
    {"website", S3OperationCode::website},
    {"analytics", S3OperationCode::analytics},
    {"inventory", S3OperationCode::inventory},
    {"metrics", S3OperationCode::metrics},
    {"replication", S3OperationCode::replication},
    {"accelerate", S3OperationCode::accelerate},
    {"versions", S3OperationCode::versions},
    {"delete", S3OperationCode::multidelete},
    {"uploads", S3OperationCode::multipart},
    {"uploadId", S3OperationCode::multipart},
    {"torrent", S3OperationCode::torrent},
    {"select", S3OperationCode::selectcontent},
    {"restore", S3OperationCode::restore}
    // {"selectcontent", S3OperationCode::selectcontent},
    // {"initupload", S3OperationCode::initupload},
    // {"partupload", S3OperationCode::partupload},
    // {"completeupload", S3OperationCode::completeupload},
    // {"abortupload", S3OperationCode::abortupload},
    // {"multidelete", S3OperationCode::multidelete},
    // {"listuploads", S3OperationCode::listuploads},
    // {"listuploads", S3OperationCode::listuploads}
};