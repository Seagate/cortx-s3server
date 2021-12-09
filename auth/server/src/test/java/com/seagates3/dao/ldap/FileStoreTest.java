package com.seagates3.dao.ldap;

import java.io.FileOutputStream;
import java.io.ObjectOutputStream;
import java.util.HashMap;
import java.util.Map;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import com.seagates3.exception.DataAccessException;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({FileStore.class, ObjectOutputStream.class})
    @PowerMockIgnore({"javax.management.*"}) public class FileStoreTest {

 private
  FileStore mockFileStore;
 private
  FileOutputStream mockFileOutputStream;
 private
  ObjectOutputStream mockObjectOutputStream;
 private
  Object mockObject;
 private
  Map sampleMap = new HashMap();

  @Before public void setUp() throws Exception {
    PowerMockito.mockStatic(ObjectOutputStream.class);
    mockFileStore = Mockito.mock(FileStore.class);
    mockFileOutputStream = Mockito.mock(FileOutputStream.class);
    mockObjectOutputStream = PowerMockito.mock(ObjectOutputStream.class);
    mockObject = Mockito.mock(Object.class);
    sampleMap.put("p1#acc1", mockObject);
    sampleMap.put("p2#acc1", mockObject);
    sampleMap.put("p3#acc2", mockObject);
  }

  @Test public void testSave() throws Exception {
    WhiteboxImpl.setInternalState(mockFileStore, "savedDataMap", sampleMap);
    PowerMockito.whenNew(FileOutputStream.class)
        .withArguments(Mockito.anyString())
        .thenReturn(mockFileOutputStream);
    PowerMockito.whenNew(ObjectOutputStream.class)
        .withArguments(mockFileOutputStream)
        .thenReturn(mockObjectOutputStream);
    PowerMockito.doNothing().when(mockObjectOutputStream).writeObject(
        sampleMap);
    PowerMockito.doNothing().when(mockObjectOutputStream).close();
    Mockito.doCallRealMethod().when(mockFileStore).save(Mockito.anyMap(),
                                                        Mockito.anyString());
    Map newDataMap = new HashMap();
    newDataMap.put("p4#acc3", mockObject);
    mockFileStore.save(newDataMap, "Policy");
    Assert.assertEquals(4, mockFileStore.savedDataMap.size());
  }

  @Test public void testFind() throws DataAccessException {
    WhiteboxImpl.setInternalState(mockFileStore, "savedDataMap", sampleMap);
    Mockito.doCallRealMethod().when(mockFileStore).find(
        Mockito.anyString(), Mockito.anyObject(), Mockito.anyObject(),
        Mockito.anyString());
    Assert.assertNotNull(mockFileStore.find("p1#acc1", null, null, "Policy"));
  }

  @Test public void testFindAll() throws DataAccessException {
	Map<String, Object> parameters = new HashMap<>();
    WhiteboxImpl.setInternalState(mockFileStore, "savedDataMap", sampleMap);
    Mockito.doCallRealMethod().when(mockFileStore).findAll(
        Mockito.anyString(), Mockito.anyObject(), Mockito.anyMap(), Mockito.anyString());
    Assert.assertEquals(2,
                        mockFileStore.findAll("acc1", null, parameters, "Policy").size());
  }

  @Test public void testDelete() throws Exception {
    WhiteboxImpl.setInternalState(mockFileStore, "savedDataMap", sampleMap);
    PowerMockito.whenNew(FileOutputStream.class)
        .withArguments(Mockito.anyString())
        .thenReturn(mockFileOutputStream);
    PowerMockito.whenNew(ObjectOutputStream.class)
        .withArguments(mockFileOutputStream)
        .thenReturn(mockObjectOutputStream);
    PowerMockito.doNothing().when(mockObjectOutputStream).writeObject(
        sampleMap);
    PowerMockito.doNothing().when(mockObjectOutputStream).close();
    Mockito.doCallRealMethod().when(mockFileStore).delete (
        Mockito.anyString(), Mockito.anyObject(), Mockito.anyString());
    mockFileStore.delete ("p1#acc1", null, "Policy");
    Assert.assertEquals(2, mockFileStore.savedDataMap.size());
  }
}
