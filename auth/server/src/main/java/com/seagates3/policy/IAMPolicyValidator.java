package com.seagates3.policy;

import java.util.List;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Resource;
import com.seagates3.response.ServerResponse;

public
class IAMPolicyValidator extends PolicyValidator {

  @Override ServerResponse
  validateActionAndResource(List<Action> actionList,
                            List<Resource> resourceValues,
                            String inputResource) {
    // TODO Auto-generated method stub
    return null;
  }

  @Override ServerResponse
  validatePolicy(String inputResource, String jsonPolicy) {
    // TODO Auto-generated method stub
    return null;
  }
}
