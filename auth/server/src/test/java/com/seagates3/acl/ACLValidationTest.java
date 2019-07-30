package com.seagates3.acl;

import static org.junit.Assert.assertEquals;

import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class) @PrepareForTest({ACLValidation.class})
    @PowerMockIgnore({"javax.management.*"}) public class ACLValidationTest {

  // valid ACL XML
  static String acpXmlString =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
      "<AccessControlPolicy " +
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + " <Owner>" +
      "  <ID>id</ID>" + "  <DisplayName>owner</DisplayName>" + " </Owner>" +
      " <AccessControlList>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" +
      " </AccessControlList>" + "</AccessControlPolicy>";

  // invalid ACL XML schema string
  static String acpXmlString_SchemaError =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
      "<AccessControlPolicy " +
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + " <Owner>" +
      "  <ID>id</ID>" + "  <DisplayName>owner</DisplayName>" + " </Owner>" +
      " <AccessControlList>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id1</ID>" +
      "    <DisplayName>name1</DisplayName>" + "   </Grantee>" +
      "   <Permission>READ</Permission>" + "  </Grant>" +
      " </AccessControlList>" + "</AccessControlPolicy>";

 private
  ACLValidation aclValidation;

  @BeforeClass public static void setUpBeforeClass() throws Exception {
    AuthServerConfig.authResourceDir = "../resources";
  }

  // test for valid ACL XML
  @Test public void testValidate_Successful() throws Exception {

    aclValidation = new ACLValidation(acpXmlString);
    PowerMockito.mockStatic(ACLValidation.class);
    PowerMockito.when(ACLValidation.class, "checkIdExists", "id", "owner")
        .thenReturn(true);
    ServerResponse response = aclValidation.validate();
    assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  // test for invalid schema of ACL XML
  @Test public void testValidate_invalid_acl_schema() throws Exception {

    aclValidation = new ACLValidation(acpXmlString_SchemaError);
    ServerResponse response = aclValidation.validate();
    assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
  }

  // test for invalid id of ACL XML
  @Test public void testValidate_invalid_acl_id() throws Exception {

    aclValidation = new ACLValidation(acpXmlString);
    PowerMockito.mockStatic(ACLValidation.class);
    PowerMockito.when(ACLValidation.class, "checkIdExists", "id", "owner")
        .thenReturn(false);
    ServerResponse response = aclValidation.validate();
    assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
  }

  // test for invalid id of ACL XML
  @Test public void testValidate_invalid_acl_grantfull() throws Exception {

    aclValidation = new ACLValidation(acpXmlString_GrantOverflow);
    PowerMockito.mockStatic(ACLValidation.class);
    PowerMockito.when(ACLValidation.class, "checkIdExists", "id", "owner")
        .thenReturn(true);
    ServerResponse response = aclValidation.validate();
    assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
  }

  // ACL String with >100 grants
  static String acpXmlString_GrantOverflow =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
      "<AccessControlPolicy " +
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + " <Owner>" +
      "  <ID>id</ID>" + "  <DisplayName>owner</DisplayName>" + " </Owner>" +
      " <AccessControlList>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" + "  <Grant>" +
      "   <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" +
      " xsi:type=\"CanonicalUser\">" + "    <ID>id</ID>" +
      "    <DisplayName>owner</DisplayName>" + "   </Grantee>" +
      "   <Permission>FULL_CONTROL</Permission>" + "  </Grant>" +
      " </AccessControlList>" + "</AccessControlPolicy>";
}
