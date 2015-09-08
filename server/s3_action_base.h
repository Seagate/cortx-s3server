
#include <vector>

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class S3Action {
protected:
  S3RequestObject* request;
  // Any action specific state should be managed by derived classes.
private:
  // Holds the member functions that will process the request.
  // member function signature should be void fn();
  std::vector<std::function<void()>> task_list;
  int task_iteration_index;

public:
  S3Action(S3RequestObject* req);
  virtual ~S3Action();

protected:
  void add_task(std::function<void()> task) {
    task_list.push_back(task);
  }

public:
  // Register all the member functions required to complete the action.
  // This can register the function as
  // task_list.push_back(std::bind( &S3SomeDerivedAction::step1, this ));
  // Ensure you call this in Derived class constructor.
  virtual void setup_steps() = 0;

  // Common Actions.
  void start();
  // Step to next async step.
  void next();
  void done();
  void pause();
  void resume();
  void abort();

  // Common steps for all Actions like Authenticate.
  void check_authentication();
  void send_response_to_s3_client();
}
