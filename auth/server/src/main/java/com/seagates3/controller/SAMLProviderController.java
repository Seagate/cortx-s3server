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

package com.seagates3.controller;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.SAMLProviderDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.exception.SAMLInitializationException;
import com.seagates3.exception.SAMLInvalidCertificateException;
import com.seagates3.exception.SAMLReponseParserException;
import com.seagates3.model.Requestor;
import com.seagates3.model.SAMLProvider;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.SAMLProviderResponseGenerator;
import com.seagates3.saml.SAMLUtil;
import com.seagates3.saml.SAMLUtilFactory;
import java.util.Map;
import javax.management.InvalidAttributeValueException;

public class SAMLProviderController extends AbstractController {

    SAMLProviderDAO samlProviderDao;
    SAMLProviderResponseGenerator samlProviderResponseGenerator;

    public SAMLProviderController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        samlProviderDao = (SAMLProviderDAO) DAODispatcher.getResourceDAO(
                DAOResource.SAML_PROVIDER);
        samlProviderResponseGenerator = new SAMLProviderResponseGenerator();
    }

    /**
     * Create a SAML Provider.
     *
     * @return ServerResponse.
     */
    @Override
    public ServerResponse create() {
        String samlMetadata = requestBody.get("SAMLMetadataDocument");
        SAMLProvider samlProvider;
        try {
            samlProvider = samlProviderDao.find(
                    requestor.getAccount(), requestBody.get("Name"));
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        if (samlProvider.exists()) {
            return samlProviderResponseGenerator.entityAlreadyExists();
        }

        samlProvider.setSamlMetadata(samlMetadata);
        SAMLUtil samlutil;
        try {
            samlutil = SAMLUtilFactory.getSAMLUtil(samlMetadata);
        } catch (SAMLInitializationException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        try {
            samlutil.getSAMLProvider(samlProvider, samlMetadata);
        } catch (SAMLReponseParserException ex) {
            return samlProviderResponseGenerator.invalidParametervalue();
        } catch (InvalidAttributeValueException |
                SAMLInvalidCertificateException ex) {
            return samlProviderResponseGenerator.invalidParametervalue();
        }

        try {
            samlProviderDao.save(samlProvider);
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        return samlProviderResponseGenerator.generateCreateResponse(
                samlProvider);
    }

    /**
     * Delete SAML provider.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse delete() {
        String samlProviderName = getSAMLProviderName(
                requestBody.get("SAMLProviderArn"));

        SAMLProvider samlProvider;
        try {
            samlProvider = samlProviderDao.find(
                    requestor.getAccount(), samlProviderName);
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        if (!samlProvider.exists()) {
            return samlProviderResponseGenerator.noSuchEntity();
        }

        try {
            samlProviderDao.delete(samlProvider);
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        return samlProviderResponseGenerator.generateDeleteResponse();
    }

    /**
     * List all the SAML providers of an account.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse list() {
        SAMLProvider[] samlProviderList;
        try {
            samlProviderList = samlProviderDao.findAll(
                    requestor.getAccount());
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        return samlProviderResponseGenerator.generateListResponse(
                samlProviderList);
    }

    /**
     * Update the SAML Provider metadata.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse update() {
        String samlProviderName = getSAMLProviderName(
                requestBody.get("SAMLProviderArn"));

        SAMLProvider samlProvider;

        try {
            samlProvider = samlProviderDao.find(
                    requestor.getAccount(), samlProviderName);
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        if (!samlProvider.exists()) {
            return samlProviderResponseGenerator.noSuchEntity();
        }

        try {
            samlProviderDao.update(samlProvider,
                    requestBody.get("SAMLMetadataDocument"));
        } catch (DataAccessException ex) {
            return samlProviderResponseGenerator.internalServerError();
        }

        return samlProviderResponseGenerator.generateUpdateResponse(
                samlProviderName);
    }

    /**
     * TODO Write a generic implementation to parse ARN.
     *
     * Get the SAML provider name from ARN. ARN should be in this format
     * arn:seagate:iam::<account name>:<idp provider name>
     *
     * @param arn SAML provider ARN
     * @return SAML provider name
     */
    private String getSAMLProviderName(String arn) {
        String[] tokens = arn.split("arn:seagate:iam::");
        tokens = tokens[1].split(":");
        return tokens[1];
    }
}
