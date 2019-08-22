/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 04-Apr-2019
 */

package com.seagates3.acl;
import java.io.*;
import java.io.IOException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;
import org.xml.sax.InputSource;

import com.seagates3.acl.AccessControlList;
import com.seagates3.exception.GrantListFullException;
import com.seagates3.util.XMLValidatorUtil;

public
class AccessControlPolicy {

 private
  Document doc;
  Owner owner;
  AccessControlList accessControlList;

 private
  final static Logger LOGGER =
      LoggerFactory.getLogger(XMLValidatorUtil.class.getName());

 public
  AccessControlPolicy(File xmlFile) throws ParserConfigurationException,
      SAXException, IOException, GrantListFullException {

    DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
    DocumentBuilder dBuilder;
    dBuilder = dbFactory.newDocumentBuilder();
    doc = dBuilder.parse(xmlFile);
    doc.getDocumentElement().normalize();
    loadXml(doc);
  }

 public
  AccessControlPolicy(String xmlString) throws ParserConfigurationException,
      SAXException, IOException, GrantListFullException {

    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    DocumentBuilder builder = null;
    builder = factory.newDocumentBuilder();
    doc = builder.parse(new InputSource(new StringReader(xmlString)));
    loadXml(doc);
  }

  // Load ACL elements from XML doc.
 private
  void loadXml(Document doc) throws GrantListFullException {

    Owner owner = new Owner("", "");
    NodeList ownerNodes = doc.getElementsByTagName("Owner");
    Node firstOwnerNode = ownerNodes.item(0);
    Element firstOwnerElement = (Element)firstOwnerNode;
    Node ownerId = firstOwnerElement.getElementsByTagName("ID").item(0);
    owner.setCanonicalId(ownerId.getTextContent());
    Node ownerDisplayName =
        firstOwnerElement.getElementsByTagName("DisplayName").item(0);
    if (ownerDisplayName != null) {
    owner.setDisplayName(ownerDisplayName.getTextContent());
    } else {

      owner.setDisplayName(null);
    }
    this.owner = owner;
    AccessControlList accessControlList = new AccessControlList();
    NodeList accessContolListNodes =
        doc.getElementsByTagName("AccessControlList");
    Node firstAccessControlListnode = accessContolListNodes.item(0);
    Element firstAccessControlListElement = (Element)firstAccessControlListnode;
    NodeList Grant =
        firstAccessControlListElement.getElementsByTagName("Grant");

    // Iterates over GrantList
    for (int counter = 0; counter < Grant.getLength(); counter++) {

      Node grantitem = Grant.item(counter);

      Element grantitemelement = (Element)grantitem;

      Grantee grantee = new Grantee("", "");

      Node granteeId = grantitemelement.getElementsByTagName("ID").item(0);

      grantee.setCanonicalId(granteeId.getTextContent());

      Node granteeDisplayname =
          grantitemelement.getElementsByTagName("DisplayName").item(0);

      if (granteeDisplayname != null) {
      grantee.setDisplayName(granteeDisplayname.getTextContent());
      } else {

        grantee.setDisplayName(null);
      }
      Grant grant = new Grant(grantee, "");
      grant.grantee = grantee;
      Node permission =
          grantitemelement.getElementsByTagName("Permission").item(0);
      grant.setPermission(permission.getTextContent());
      accessControlList.addGrant(grant);
    }
    this.accessControlList = accessControlList;
  }

  void setOwner(Owner newOwner) {
    owner.canonicalId = newOwner.getCanonicalId();
    owner.displayName = newOwner.getDisplayName();
  }

  Owner getOwner() { return owner; }

  AccessControlList getAccessControlList() { return accessControlList; }

  void setAccessControlList(AccessControlList acl)
      throws GrantListFullException {
    this.accessControlList.clearGrantList();
    for (int counter = 0; counter < acl.getGrantList().size(); counter++) {
      this.accessControlList.addGrant(acl.getGrantList().get(counter));
    }
  }

  /**
   * Creates a default {@link AccessControlPolicy} for object
   * @param requestor
* @throws GrantListFullException
   */
 public
  void initDefaultACL(String canonicalId,
                      String name) throws GrantListFullException {
    owner = new Owner(canonicalId, name);
    accessControlList = new AccessControlList();
    Grantee grantee = new Grantee(canonicalId, name);
    Grant grant = new Grant(grantee, "FULL_CONTROL");
    accessControlList.addGrant(grant);
  }

  // Returns ACL XML in string buffer.
 public
  String getXml() throws TransformerException {
    TransformerFactory tf = TransformerFactory.newInstance();
    Transformer transformer;
    transformer = tf.newTransformer();
    flushXmlValues();
    StringWriter writer = new StringWriter();
    transformer.transform(new DOMSource(doc), new StreamResult(writer));
    String xml = writer.getBuffer().toString();
    return xml;
  }

 private
  void flushXmlValues() {
    NodeList ownerNodes = doc.getElementsByTagName("Owner");
    Node firstOwnerNode = ownerNodes.item(0);
    Element firstOwnerElement = (Element)firstOwnerNode;

    Node ownerIdNode = firstOwnerElement.getElementsByTagName("ID").item(0);
    ownerIdNode.setTextContent(this.owner.getCanonicalId());

    Node ownerDisplayNameNode =
        firstOwnerElement.getElementsByTagName("DisplayName").item(0);
    ownerDisplayNameNode.setTextContent(this.owner.getDisplayName());

    Node accessControlListNode =
        doc.getElementsByTagName("AccessControlList").item(0);
    Element firstAccessControlListElement = (Element)accessControlListNode;

    NodeList GrantNodeList =
        firstAccessControlListElement.getElementsByTagName("Grant");

    int grantNodesLength = GrantNodeList.getLength();
    int counter = 0;

    for (counter = 0; counter < this.accessControlList.getGrantList().size();
         counter++) {

      // When grantNodeList length is less than grantlength to be set,adding
      // extra nodes of type grant Node.
      if (counter > (grantNodesLength - 1)) {
        Node grantNode = GrantNodeList.item(0);
        Node newNode = grantNode.cloneNode(true);
        accessControlListNode.appendChild(newNode);
      }

      Node grantNode = GrantNodeList.item(counter);
      Element grantElement = (Element)grantNode;

      Node granteeIdNode = grantElement.getElementsByTagName("ID").item(0);
      granteeIdNode.setTextContent(this.accessControlList.getGrantList()
                                       .get(counter)
                                       .grantee.getCanonicalId());

      Node granteeDisplayNameNode =
          grantElement.getElementsByTagName("DisplayName").item(0);
      granteeDisplayNameNode.setTextContent(
          this.accessControlList.getGrantList()
              .get(counter)
              .grantee.getDisplayName());

      Node permissionNode =
          grantElement.getElementsByTagName("Permission").item(0);
      permissionNode.setTextContent(
          this.accessControlList.getGrantList().get(counter).getPermission());
    }

    int indexAtNodeToBeDeleted = counter;

    // When grantNode length is greater than grantlength to be set,removing
    // nodes of type Grant.
    if (grantNodesLength > counter) {
      while ((grantNodesLength) != counter) {
        Node deleteNode = GrantNodeList.item(indexAtNodeToBeDeleted);
        accessControlListNode.removeChild(deleteNode);
        counter++;
      }
    }
  }
}
