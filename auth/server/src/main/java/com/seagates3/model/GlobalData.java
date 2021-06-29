package com.seagates3.model;

public
class GlobalData {

 private
  AccessKey accessKey;
 private
  Requestor requestor;
 private
  long creationTime;

 public
  GlobalData(AccessKey key, Requestor requestor, long time) {
    this.accessKey = key;
    this.requestor = requestor;
    this.creationTime = time;
  }

 public
  AccessKey getAccessKey() { return accessKey; }
 public
  void setAccessKey(AccessKey accessKey) { this.accessKey = accessKey; }
 public
  Requestor getRequestor() { return requestor; }
 public
  void setRequestor(Requestor requestor) { this.requestor = requestor; }
 public
  long getCreationTime() { return creationTime; }
 public
  void setCreationTime(long creationTime) { this.creationTime = creationTime; }
}
