package com.seagates3.dao.ldap;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public
class FileStore implements AuthStore {

  Map<String, Policy> policyDetailsMap = null;
 private
  final Logger LOGGER = LoggerFactory.getLogger(FileStore.class.getName());

 public
  FileStore() {
    try {
      FileInputStream fis = new FileInputStream("/tmp/policydata.ser");
      ObjectInputStream ois = new ObjectInputStream(fis);
      policyDetailsMap = (Map)ois.readObject();
      ois.close();
    }
    catch (FileNotFoundException fe) {
      LOGGER.error(
          "FileNotFoundException occurred while reading policy file - " + fe);
      policyDetailsMap = new HashMap<>();
    }
    catch (IOException | ClassNotFoundException e) {
      LOGGER.error("Exception occurred while reading policy from file - " + e);
    }
  }

  @Override public void save(Policy policy) throws DataAccessException {
    String key =
        FileStoreUtil.getKey(policy.getName(), policy.getAccount().getId());
    policyDetailsMap.put(key, policy);
    try {
      FileOutputStream fos = new FileOutputStream("/tmp/policydata.ser");
      ObjectOutputStream oos = new ObjectOutputStream(fos);
      oos.writeObject(policyDetailsMap);
      oos.close();
    }
    catch (IOException e) {
      LOGGER.error("Exception occurred while saving policy into file - " + e);
    }
  }

  @Override public Policy find(String policyarn) throws DataAccessException {
    if (policyarn != null) {
      String key = FileStoreUtil.retrieveKeyFromArn(policyarn);
      return policyDetailsMap.get(key);
    }
    return null;
  }

  @Override public Policy find(Account account,
                               String name) throws DataAccessException {
    String key = FileStoreUtil.getKey("", account.getId());
    for (Entry<String, Policy> entry : policyDetailsMap.entrySet()) {
      if (entry.getKey().contains(key) &&
          name.equals(entry.getValue().getName())) {
        return entry.getValue();
      }
    }
    return null;
  }

  @Override public List<Policy> findAll(Account account)
      throws DataAccessException {
    List<Policy> policyList = new ArrayList<>();
    String key = FileStoreUtil.getKey("", account.getId());
    for (Entry<String, Policy> entry : policyDetailsMap.entrySet()) {
      if (entry.getKey().contains(key)) {
        policyList.add(entry.getValue());
      }
    }
    return policyList;
  }

  @Override public void delete (Policy policy) throws DataAccessException {
    String keyToBeRemoved =
        FileStoreUtil.getKey(policy.getName(), policy.getAccount().getId());
    policyDetailsMap.remove(keyToBeRemoved);
    try {
      FileOutputStream fos = new FileOutputStream("/tmp/policydata.ser");
      ObjectOutputStream oos = new ObjectOutputStream(fos);
      oos.writeObject(policyDetailsMap);
      oos.close();
    }
    catch (IOException e) {
      LOGGER.error("Exception occurred while saving policy into file - " + e);
    }
  }
}
