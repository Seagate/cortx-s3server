/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "s3_delete_multiple_objects_body.h"
#include "s3_log.h"
#include "s3_fi_common.h"
#include "s3_option.h"
#include "s3_iem.h"

#include <algorithm>

S3DeleteMultipleObjectsBody::S3DeleteMultipleObjectsBody()
    : is_valid(false), quiet(false), request() {
  s3_log(S3_LOG_DEBUG, "", "%s Ctor\n", __func__);
}

void S3DeleteMultipleObjectsBody::initialize(
    std::shared_ptr<S3RequestObject> req, std::string &xml) {
  request = req;
  xml_content = xml;
  parse_and_validate();
}

bool S3DeleteMultipleObjectsBody::isOK() { return is_valid; }

bool S3DeleteMultipleObjectsBody::parse_and_validate() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  /* Sample body:
  <?xml version="1.0" encoding="UTF-8"?>
  <Delete>
      <Quiet>true</Quiet>
      <Object>
           <Key>Key</Key>
           <VersionId>VersionId</VersionId>
      </Object>
      <Object>
           <Key>Key</Key>
      </Object>
      ...
  </Delete>
  */
  if (xml_content.empty()) {
    is_valid = false;
    return false;
  }
  s3_log(S3_LOG_DEBUG, "", "Parsing xml request = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar *)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_DEBUG, "", "XML request body Invalid.\n");
    is_valid = false;
    return false;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);

  // Validate the root node
  if (root_node == NULL ||
      xmlStrcmp(root_node->name, (const xmlChar *)"Delete")) {
    s3_log(S3_LOG_ERROR, "", "XML request body Invalid.\n");
    xmlFreeDoc(document);
    is_valid = false;
    return false;
  }

  // Get the request attributes
  xmlNodePtr child = root_node->xmlChildrenNode;
  xmlChar *key = NULL;
  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"Quiet"))) {
      key = xmlNodeGetContent(child);
      if (key == NULL) {
        s3_log(S3_LOG_ERROR, "", "XML request body Invalid.\n");
      } else if ((!xmlStrcmp(key, (const xmlChar *)"true"))) {
        quiet = true;
      }
      if (key != NULL) {
        xmlFree(key);
      }
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"Object"))) {
      s3_log(S3_LOG_DEBUG, "", "Found child in xml = Object\n");
      // Read delete object details
      xmlChar *obj_key = NULL;
      xmlChar *ver_id = NULL;

      xmlNodePtr obj_child = child->xmlChildrenNode;
      while (obj_child != NULL) {
        s3_log(S3_LOG_DEBUG, "", "Found child in xml = %s\n",
               (const char *)obj_child->name);
        if ((!xmlStrcmp(obj_child->name, (const xmlChar *)"Key"))) {
          obj_key = xmlNodeGetContent(obj_child);
          if (obj_key == NULL) {
            s3_log(S3_LOG_DEBUG, "", "Object key missing in request.\n");
            if (ver_id != NULL) {
              xmlFree(ver_id);
            }
            xmlFreeDoc(document);
            is_valid = false;
            return false;
          }
        } else if ((!xmlStrcmp(obj_child->name,
                               (const xmlChar *)"VersionId"))) {
          ver_id = xmlNodeGetContent(obj_child);
          if (ver_id == NULL) {
            s3_log(S3_LOG_DEBUG, "", "Version ID missing in request.\n");
          }
        }
        obj_child = obj_child->next;
      }  // obj_child while
      // Remember the object to delete.
      if (obj_key == NULL) {
        xmlFreeDoc(document);
        is_valid = false;
        return false;
      } else {
        object_keys.push_back(((const char *)obj_key));
      }

      if (ver_id == NULL) {
        version_ids.push_back("");
      } else {
        version_ids.push_back(((const char *)ver_id));
      }

      // object_list.emplace_back(new DeleteObjectInfo((const char* obj_key),
      // (const char*)ver_id));
      s3_log(S3_LOG_DEBUG, "", "Object to delete = %s\n",
             (const char *)obj_key);
      if (obj_key != NULL) {
        xmlFree(obj_key);
      }
      if (ver_id != NULL) {
        xmlFree(ver_id);
      }
    }  // Object
    child = child->next;
  }

  is_valid = true;
  xmlFreeDoc(document);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return is_valid;
}

bool S3DeleteMultipleObjectsBody::validate_attrs(const std::string &bckt,
                                                 const std::string &key) {
  bool fi_en = s3_fi_is_enabled("di_metadata_bucket_or_object_corrupted");
  std::string body_bucket = request ? request->get_bucket_name() : "";
  bool is_bckt = (bckt == body_bucket);
  bool is_key = (std::find(std::begin(object_keys), std::end(object_keys),
                           key) != std::end(object_keys));
  bool ret = !fi_en || (is_bckt && is_key);

  if (!ret) {
    if (!S3Option::get_instance()
             ->get_s3_di_disable_metadata_corruption_iem()) {
      std::string body_accname = request ? request->get_account_name() : "";
      s3_iem(LOG_ERR, S3_IEM_OBJECT_METADATA_NOT_VALID,
             S3_IEM_OBJECT_METADATA_NOT_VALID_STR,
             S3_IEM_OBJECT_METADATA_NOT_VALID_JSON, body_bucket.c_str(),
             bckt.c_str(), "<delete-multiple-objects-list>", key.c_str(),
             body_accname.c_str());
    } else {
      s3_log(S3_LOG_ERROR, "",
             "Object metadata mismatch: "
             "req_bucket_name=\"%s\" c_bucket_name=\"%s\" "
             "req_object_name=\"%s\" c_object_name=\"%s\"",
             body_bucket.c_str(), bckt.c_str(),
             "<delete-multiple-objects-list>", key.c_str());
    }
  }
  return ret;
}
