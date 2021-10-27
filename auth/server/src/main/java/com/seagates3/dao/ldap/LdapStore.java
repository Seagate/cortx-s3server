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

  @Override public void save(Map<String, Object> dataMap, Object obj,
                             String prefix) throws DataAccessException {

    String methodName = "save";
    String className = "com.seagates3.dao.ldap." + prefix + "LdapStore";
    LOGGER.debug("calling method - " + methodName + " of class - " + className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(methodName, Object.class);
      method.invoke(instance, obj);
    }
    catch (ClassNotFoundException e) {
    }
    catch (NoSuchMethodException | SecurityException e) {
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
    }
  }

  @Override public Object find(String strToFind, Object obj,
                               String prefix) throws DataAccessException {

    String methodName = "find";
    String className = "com.seagates3.dao.ldap." + prefix + "LdapStore";
    LOGGER.debug("calling method - " + methodName + " of class - " + className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(methodName, Object.class);
      Object returnObj = method.invoke(instance, obj);
      return returnObj;
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("ClassNotFoundException" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("NoSuchMethodException" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("IllegalArgumentException " + e);
    }
    return null;
  }

  @Override public List findAll(String strToFind, Object obj,
                                String prefix) throws DataAccessException {
    String methodName = "findAll";
    String className = "com.seagates3.dao.ldap." + prefix + "LdapStore";
    LOGGER.debug("calling method - " + methodName + " of class - " + className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(methodName, Object.class);
      return (List)method.invoke(instance, obj);
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("ClassNotFoundException" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("NoSuchMethodException" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("IllegalArgumentException " + e);
    }
    return null;
  }

  @Override public void delete (String key, Object obj,
                                String prefix) throws DataAccessException {

    String methodName = "delete";
    String className = "com.seagates3.dao.ldap." + prefix + "LdapStore";
    LOGGER.debug("calling method - " + methodName + " of class - " + className);
    try {
      Class < ? > storeClass = Class.forName(className);
      Object instance = storeClass.newInstance();
      Method method = storeClass.getMethod(methodName, Object.class);
      method.invoke(instance, obj);
    }
    catch (ClassNotFoundException e) {
      LOGGER.error("ClassNotFoundException" + e);
    }
    catch (NoSuchMethodException | SecurityException e) {
      LOGGER.error("NoSuchMethodException" + e);
    }
    catch (IllegalAccessException | IllegalArgumentException |
           InvocationTargetException | InstantiationException e) {
      LOGGER.error("IllegalArgumentException " + e);
    }
  }
}

