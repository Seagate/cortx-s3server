package com.seagates3.model;

public
class UserPolicy {

 private
  User user;
 private
  Policy policy;

 public
  UserPolicy() {}

 public
  UserPolicy(User user, Policy policy) {
    this.user = user;
    this.policy = policy;
  }

 public
  User getUser() { return user; }

 public
  void setUser(User user) { this.user = user; }

 public
  Policy getPolicy() { return policy; }

 public
  void setPolicy(Policy policy) { this.policy = policy; }
}
