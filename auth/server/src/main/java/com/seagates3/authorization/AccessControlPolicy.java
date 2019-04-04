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

package com.seagates3.authorization;
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

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.seagates3.authorization.AccessControlList;

public class AccessControlPolicy {

    private static Document doc;
    Owner owner;
    AccessControlList accessControlList;

    public AccessControlPolicy(String aclXmlPath) throws ParserConfigurationException,
                      SAXException, IOException {

        File xmlFile = new File(aclXmlPath);
        DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
        DocumentBuilder dBuilder;
        dBuilder = dbFactory.newDocumentBuilder();
        doc = dBuilder.parse(xmlFile);
        doc.getDocumentElement().normalize();

        loadXml(doc);
    }

    // Load ACL elements from XML doc.
    private void loadXml(Document doc) {

        Owner owner = new Owner("", "");
        NodeList ownerNodes = doc.getElementsByTagName("Owner");
        Node firstOwnerNode = ownerNodes.item(0);
        Element firstOwnerElement = (Element)firstOwnerNode;
        Node ownerId = firstOwnerElement.getElementsByTagName("ID").item(0);
        owner.setCanonicalId(ownerId.getTextContent());
        Node ownerDisplayName = firstOwnerElement.getElementsByTagName("DisplayName").item(0);
        owner.setDisplayName(ownerDisplayName.getTextContent());
        this.owner = owner;

        AccessControlList accessControlList = new AccessControlList();
        NodeList accessContolListNodes = doc.getElementsByTagName("AccessControlList");
        Node firstAccessControlListnode = accessContolListNodes.item(0);
        Element firstAccessControlListElement = (Element)firstAccessControlListnode;
        NodeList Grant = firstAccessControlListElement.getElementsByTagName("Grant");

        //Iterates over GrantList
        for (int counter = 0; counter< Grant.getLength(); counter++) {

            Node grantitem = Grant.item(counter);
            Element grantitemelement = (Element)grantitem;
            Grantee grantee = new Grantee("", "");

            Node granteeId = grantitemelement.getElementsByTagName("ID").item(0);
            grantee.setCanonicalId(granteeId.getTextContent());

            Node granteeDisplayname = grantitemelement.getElementsByTagName("DisplayName").item(0);
            grantee.setDisplayName(granteeDisplayname.getTextContent());

            Grant grant = new Grant(grantee, "");
            grant.grantee = grantee;


            Node permission = grantitemelement.getElementsByTagName("Permission").item(0);
            grant.setPermission(permission.getTextContent());
            accessControlList.grantList.add(grant);
        }

        this.accessControlList = accessControlList;

   }

   void setOwner(Owner owner) {
   }

   Owner getOwner() {
       return owner;
   }

   AccessControlList getAccessControlList() {
      return accessControlList;
   }

   void setAccessControlList(AccessControlList accessControlList) {
   }

   // Returns ACL XML in string buffer.
   String getXml() throws TransformerException  {
       TransformerFactory tf = TransformerFactory.newInstance();
       Transformer transformer;
       transformer = tf.newTransformer();
       StringWriter writer = new StringWriter();
       transformer.transform(new DOMSource(doc), new StreamResult(writer));
       String xml = writer.getBuffer().toString();
       return xml;
   }
}
