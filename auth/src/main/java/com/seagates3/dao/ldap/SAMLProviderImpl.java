/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 15-Oct-2015
 */

package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPAttributeSet;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.novell.ldap.LDAPSearchResults;

import com.seagates3.dao.SAMLProvicerDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.SAMLProvider;
import com.seagates3.model.KeyDescriptor;
import com.seagates3.util.BinaryUtil;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SAMLProviderImpl implements SAMLProvicerDAO {

    /*
     * Get the IDP descriptor from LDAP.
     */
    @Override
    public SAMLProvider find(String accountName, String name) throws DataAccessException {
        SAMLProvider samlProvider = new SAMLProvider();
        samlProvider.setAccountName(accountName);
        samlProvider.setName(name);

        String[] attrs = {"samlmetadata", "exp"};
        String ldapBase = String.format("name=%s,ou=idp,o=%s,ou=accounts,%s", name,
                accountName, LdapUtils.getBaseDN());
        String filter = String.format("(name=%s)", name);

        LDAPSearchResults ldapResults;
        try {
            ldapResults = LdapUtils.search(ldapBase,
                    LDAPConnection.SCOPE_SUB, filter, attrs);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to find idp.\n" + ex);
        }

        if(ldapResults.hasMore()) {
            try {
                LDAPEntry entry = ldapResults.next();
                samlProvider.setSamlMetadata(entry.getAttribute("samlmetadata").getStringValue());
                samlProvider.setExpiry(entry.getAttribute("exp").getStringValue());

            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to find idp.\n" + ex);
            }
        }

        return samlProvider;
    }

    /*
     * Get the list of all the saml providers.
     */
    public SAMLProvider[] findAll(String accountName) throws DataAccessException {
        ArrayList samlProviders = new ArrayList();
        return null;
    }

    /*
     * Save the new identity provider.
     */
    @Override
    public void save(SAMLProvider samlProvider) throws DataAccessException {
        try {
            createNewProvider(samlProvider);
            createKeyDescriptors(samlProvider);
        } catch (CertificateEncodingException ex) {
            Logger.getLogger(SAMLProviderImpl.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    @Override
    public void delete(SAMLProvider samlProvider) throws DataAccessException {
        deleteKeyDescriptors(samlProvider);
        deleteSamlProvider(samlProvider);
    }

    @Override
    public void update(SAMLProvider samlProvider, String newSamlMetadata) throws DataAccessException {
        deleteKeyDescriptors(samlProvider);
        updateSamlProvider(samlProvider, newSamlMetadata);
        try {
            createKeyDescriptors(samlProvider);
        } catch (CertificateEncodingException ex) {
            Logger.getLogger(SAMLProviderImpl.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    /*
     * Create a new entry for the identity provider.
     * Example
     * name=<provider name>,ou=idp,o=<account name>,ou=account,dc=s3,dc=seagate,dc=com
     */
    private void createNewProvider(SAMLProvider samlProvider)
            throws DataAccessException {

        LDAPAttributeSet attributeSet = new LDAPAttributeSet();
        attributeSet.add( new LDAPAttribute("objectclass", "SAMLProvider"));
        attributeSet.add( new LDAPAttribute("name", samlProvider.getName()));
        attributeSet.add( new LDAPAttribute("samlmetadata", samlProvider.getSamlMetadata()));
        attributeSet.add( new LDAPAttribute("exp", samlProvider.getExpiry()));

        String dn = String.format("name=%s,ou=idp,o=%s,ou=accounts,%s",
                samlProvider.getName(), samlProvider.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.add(new LDAPEntry(dn, attributeSet));
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to create new idp." + ex);
        }
    }

    /*
     * Create a new entry for the key descriptors belonging to an idp.
     * Example
     * samlkeyuse=<encrytion/signing>,name=<provider name>,ou=idp,
     *      o=<account name>,ou=account,dc=s3,dc=seagate,dc=com
     */
    private void createKeyDescriptors(SAMLProvider samlProvider)
            throws DataAccessException, CertificateEncodingException{

        for(KeyDescriptor k : samlProvider.getKeyDescriptors()) {
            LDAPAttributeSet attributeSet = new LDAPAttributeSet();
            attributeSet.add( new LDAPAttribute("objectclass", "SAMLKeyDescriptor"));
            attributeSet.add( new LDAPAttribute("samlkeyuse", k.getUse().toString()));

            BinaryUtil.encodeToBase64Bytes("");
            for(Certificate cert : k.getCertificate()) {
                attributeSet.add( new LDAPAttribute("cacertificate;binary", cert.getEncoded()));
            }

            String dn = String.format("samlkeyuse=%s,name=%s,ou=idp,o=%s,ou=accounts,%s",
                    k.getUse().toString(), samlProvider.getName(),
                    samlProvider.getAccountName(), LdapUtils.getBaseDN());

            try {
                LdapUtils.add(new LDAPEntry(dn, attributeSet));
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to create idp key descriptors" + ex);
            }
        }
    }

    /*
     * Delete the saml provider entry.
     */
    private void deleteSamlProvider(SAMLProvider samlProvider) throws DataAccessException {
        String dn = String.format("name=%s,ou=idp,o=%s,ou=accounts,%s",
                samlProvider.getName(), samlProvider.getAccountName(), LdapUtils.getBaseDN());

        try {
            LdapUtils.delete(dn);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to delete the user.\n" + ex);
        }
    }
    /*
     * Openldap doesn't delete recursively. hence delete key descriptors first.
     *
     * To Do
     * Add a function to delete recursively in LdapUtils.
     */
    private void deleteKeyDescriptors(SAMLProvider samlProvider) throws DataAccessException {
        String[] descriptorUse = new String[] {"encryption", "signing"};

        for(String use : descriptorUse) {
            String dn = String.format("samlkeyuse=%s,name=%s,ou=idp,o=%s,ou=accounts,%s",
                        use, samlProvider.getName(),
                        samlProvider.getAccountName(), LdapUtils.getBaseDN());

            try {
                LdapUtils.delete(dn);
            } catch (LDAPException ex) {
                throw new DataAccessException("Failed to delete the user.\n" + ex);
            }
        }
    }

    /*
     * Update saml provider metadata.
     */
    private void updateSamlProvider(SAMLProvider samlProvider,
            String samlMetadata) throws DataAccessException {
        ArrayList modList = new ArrayList();
        LDAPAttribute attr;

        String dn = String.format("name=%s,ou=idp,o=%s,ou=accounts,%s",
                samlProvider.getName(), samlProvider.getAccountName(), LdapUtils.getBaseDN());

        attr = new LDAPAttribute("samlmetadata", samlMetadata);
        modList.add(new LDAPModification(LDAPModification.REPLACE, attr));

        try {
            LdapUtils.modify(dn, modList);
        } catch (LDAPException ex) {
            throw new DataAccessException("Failed to modify the user details.\n" + ex);
        }
    }
}
