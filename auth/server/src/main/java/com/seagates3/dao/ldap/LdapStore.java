package com.seagates3.dao.ldap;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.exception.DataAccessException;

public
class LdapStore implements AuthStore {

 private
  final Logger LOGGER = LoggerFactory.getLogger(LdapStore.class.getName());
 private
  final String METHOD_SAVE = "save";
 private
  final String METHOD_FIND = "find";
 private
  final String METHOD_FINDALL = "findAll";
 private
  final String METHOD_DELETE = "delete";
 private
  final String METHOD_ATTACH = "attach";
 private
  final String METHOD_DETACH = "detach";
 private
  final String CLASS_PACKAGE = "com.seagates3.dao.ldap.";
 private
  final String CLASS_SUFFIX = "LdapStore";

  @Override public void save(Map<String, Object> dataMap,
                             String prefix) throws DataAccessException {

    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_SAVE + " of class - " +
                 className);
    Map.Entry<String, Object> entry = dataMap.entrySet().iterator().next();
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(METHOD_SAVE, Object.class);
      method.invoke(instance, entry.getValue());
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Failed to save - " + prefix);
      throw new DataAccessException("failed to save.\n" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception while calling save method");
      throw new DataAccessException("failed to call save method\n" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Failed to save - " + prefix);
      throw new DataAccessException("failed to save- " + prefix + e);
    }
  }

  @Override public Object find(String strToFind, Object obj, Object obj2,
                               String prefix) throws DataAccessException {
    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_FIND + " of class - " +
                 className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method =
          storeClass.getMethod(METHOD_FIND, Object.class, Object.class);
      Object returnObj = method.invoke(instance, obj, obj2);
      return returnObj;
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Exception occurred in find " + e);
      throw new DataAccessException("failed to find - " + prefix + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception occurred " + e);
      throw new DataAccessException("failed to find " + prefix + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Exception found " + e);
      throw new DataAccessException("failed to find " + prefix + e);
    }
  }

  @Override public List findAll(String strToFind, Object obj,
                                String prefix) throws DataAccessException {
    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_FINDALL + " of class - " +
                 className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(METHOD_FINDALL, Object.class);
      return (List)method.invoke(instance, obj);
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Exception occurred " + e);
      throw new DataAccessException("failed in findAll " + prefix + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception occurred " + e);
      throw new DataAccessException("failed in findAll " + prefix + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Exception occurred " + e);
      throw new DataAccessException("failed in findAll " + prefix + e);
    }
  }

  @Override public void delete (String key, Object obj,
                                String prefix) throws DataAccessException {
    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_DELETE + " of class - " +
                 className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(METHOD_DELETE, Object.class);
      method.invoke(instance, obj);
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Exception occurred while deleting " + prefix + e);
      throw new DataAccessException("failed while deleting " + prefix + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception occurred while deleting " + prefix + e);
      throw new DataAccessException("failed while deleting " + prefix + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Exception occurred while deleting " + prefix + e);
      throw new DataAccessException("failed while deleting " + prefix + e);
    }
  }

  @Override public void attach(Map<String, Object> dataMap,
                               String prefix) throws DataAccessException {
    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_ATTACH + " of class - " +
                 className);
    Map.Entry<String, Object> entry = dataMap.entrySet().iterator().next();
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(METHOD_ATTACH, Object.class);
      method.invoke(instance, entry.getValue());
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Failed to attach - " + prefix);
      throw new DataAccessException("failed to attach.\n" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception while calling attach method");
      throw new DataAccessException("failed to call attach method\n" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Failed to attach - " + prefix);
      throw new DataAccessException("failed to attach- " + prefix + e);
    }
  }

  @Override public void detach(Map<String, Object> dataMap,
                               String prefix) throws DataAccessException {
    String className = CLASS_PACKAGE + prefix + CLASS_SUFFIX;
    LOGGER.debug("calling method - " + METHOD_DETACH + " of class - " +
                 className);
    Map.Entry<String, Object> entry = dataMap.entrySet().iterator().next();
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(METHOD_DETACH, Object.class);
      method.invoke(instance, entry.getValue());
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("Failed to detach - " + prefix);
      throw new DataAccessException("failed to detach.\n" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("Exception while calling detach method");
      throw new DataAccessException("failed to call detach method\n" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("Failed to detach - " + prefix);
      throw new DataAccessException("failed to detach- " + prefix + e);
    }
  }
}

