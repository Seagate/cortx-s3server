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
 * Original creation date: 12-Oct-2015
 */

package com.seagates3.controller;

import java.util.Map;

import org.opensaml.xml.io.UnmarshallingException;
import org.opensaml.xml.parse.XMLParserException;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.SAMLProviderDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.SAMLProvider;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.xml.SAMLProviderResponseGenerator;
import com.seagates3.util.SAMLUtil;
import java.security.cert.CertificateException;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SAMLProviderController extends AbstractController {
    SAMLProviderDAO samlProviderDao;
    SAMLProviderResponseGenerator samlProviderResponse;

    public SAMLProviderController(Requestor requestor, Map<String, String> requestBody) {
        super(requestor, requestBody);

        samlProviderDao = (SAMLProviderDAO) DAODispatcher.getResourceDAO(DAOResource.SAML_PROVIDER);
        samlProviderResponse = new SAMLProviderResponseGenerator();
    }

    @Override
    public ServerResponse create() throws DataAccessException {
        String samlMetadata = requestBody.get("SAMLMetadataDocument");
        SAMLProvider samlProvider = samlProviderDao.find(
                requestor.getAccountName(), requestBody.get("Name"));

        if(samlProvider.exists()) {
            return samlProviderResponse.entityAlreadyExists();
        }

        samlProvider.setSamlMetadata(samlMetadata);

        try {
            SAMLUtil.getsamlProvider(samlProvider, samlMetadata);
        } catch (XMLParserException | UnmarshallingException ex) {
            throw new DataAccessException("Exception occured while creating saml provider" + ex);
        } catch (CertificateException ex) {
            Logger.getLogger(SAMLProviderController.class.getName()).log(Level.SEVERE, null, ex);
        }

        samlProviderDao.save(samlProvider);
        return samlProviderResponse.create(requestBody.get("Name"));
    }

    @Override
    public ServerResponse delete() throws DataAccessException {
        SAMLProvider samlProvider = samlProviderDao.find(
                requestor.getAccountName(), requestBody.get("SAMLProviderArn"));

        if(!samlProvider.exists()) {
            return samlProviderResponse.noSuchEntity();
        }

        samlProviderDao.delete(samlProvider);

        return samlProviderResponse.success("DeleteUser");
    }

    @Override
    public ServerResponse list() throws DataAccessException {
        SAMLProvider[] samlProviderList;
        samlProviderList = samlProviderDao.findAll(requestor.getAccountName());

        if(samlProviderList == null) {
            return samlProviderResponse.internalServerError();
        }

        return samlProviderResponse.list(samlProviderList);
    }

    /*
     * Delete the saml provider and create a new entry.
     */
    @Override
    public ServerResponse update() throws DataAccessException {
        SAMLProvider samlProvider = samlProviderDao.find(
                requestor.getAccountName(), requestBody.get("SAMLProviderArn"));

        String samlMetadata = requestBody.get("SAMLMetadataDocument");

        if(!samlProvider.exists()) {
            return samlProviderResponse.noSuchEntity();
        }

        try {
            SAMLUtil.getsamlProvider(samlProvider, samlMetadata);
        } catch (XMLParserException | UnmarshallingException ex) {
            throw new DataAccessException("Exception occured while creating saml provider" + ex);
        } catch (CertificateException ex) {
            Logger.getLogger(SAMLProviderController.class.getName()).log(Level.SEVERE, null, ex);
        }

        samlProviderDao.update(samlProvider, requestBody.get("SAMLMetadataDocument"));

        return samlProviderResponse.update();
    }
}
