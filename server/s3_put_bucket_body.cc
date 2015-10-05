
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "s3_put_bucket_body.h"

S3PutBucketBody::S3PutBucketBody(std::string& xml) : xml_content(xml), is_valid(false) {
  parse_and_validate();
}

bool S3PutBucketBody::isOK() {
  return is_valid;
}

bool S3PutBucketBody::parse_and_validate() {
  /* Sample body:
  <CreateBucketConfiguration xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    <LocationConstraint>EU</LocationConstraint>
  </CreateBucketConfiguration >
  */
  if (xml_content.empty()) {
    // Default location intended.
    location_constraint = "US";
    is_valid = true;
    return true;
  }
  printf("Parsing xml request = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar*)xml_content.c_str());
  if (document == NULL ) {
    printf("S3PutBucketBody XML request body Invalid.\n");
    is_valid = false;
    return false;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);

  // Validate the root node
  if (root_node == NULL || xmlStrcmp(root_node->name, (const xmlChar *)"CreateBucketConfiguration")) {
    printf("S3PutBucketBody XML request body Invalid.\n");
    xmlFreeDoc(document);
    is_valid = false;
    return false;
  }

  // Get location if specified.
  xmlNodePtr child = root_node->xmlChildrenNode;
  xmlChar *key = NULL;
  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"LocationConstraint"))){
      key = xmlNodeGetContent(child);
      if (key == NULL) {
        printf("S3PutBucketBody XML request body Invalid.\n");
        xmlFree(key);
        xmlFreeDoc(document);
        is_valid = false;
        return false;
      }
      location_constraint = (char*)key;
      is_valid = true;
      xmlFree(key);
      break;
    } else {
      child = child->next;
    }
  }
  xmlFreeDoc(document);
  return is_valid;
}

std::string S3PutBucketBody::get_location_constraint() {
  return location_constraint;
}
