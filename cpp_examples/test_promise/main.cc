
#include "future.h"
#include <thread>
#include <iostream>
#include <string>

using namespace pulsar;
enum class ErrorCode { kSuccess = 0, kInvalidFormat, kUserCallbackException };

inline std::string strErrorCode(ErrorCode error) {
  switch (error) {
    case ErrorCode::kSuccess:
      return "success";
    case ErrorCode::kInvalidFormat:
      return "invalid format";
    case ErrorCode::kUserCallbackException:
      return "exception from user's callback";
  }
  return "unknown error";  // 永远不会到达这里，仅仅是为了关闭编译警告
}

class IntegerParser {
 public:
  // 异步 API，用户提供回调，注意，回调的参数2是 int 而非 const int&
  Future<ErrorCode, int> parseAsync(
      const std::string& input, std::function<void(ErrorCode, int)> callback) {
    std::thread t([&input, this] {
      try {
        int number = std::stoi(input);
        promise_.setValue(number);
      } catch (...) {
        promise_.setFailed(ErrorCode::kInvalidFormat);
      }
    });
    t.detach();
    // 间接调用回调，并处理用户提供的回调可能抛出的异常
    return promise_.getFuture().addListener(
        [callback](ErrorCode code, const int& result) {
          try {
            callback(code, result);
          } catch (const std::exception& e) {
            std::cerr << "[ERROR] User's callback throws: " << e.what()
                      << std::endl;
          }
        });
  }

  // 同步 API，返回错误码，传入引用保存处理结果
  ErrorCode parse(const std::string& input, int& result) {
    auto parse_error = ErrorCode::kSuccess;
    auto fut = parseAsync(input, [&parse_error](ErrorCode error, const int&) {
      parse_error = error;
    });
    fut.get(result);
    return parse_error;
  }

 private:
  Promise<ErrorCode, int> promise_;
};

int main()
{
    IntegerParser parser;

    std::function<void(ErrorCode,int)> parse_cb = [](ErrorCode err,int input)
    {
        if(err != ErrorCode::kSuccess)
        {
            std::cout << "Parse failed" << std::endl;
            return;
        }
        std::cout << "parse end:" << input << std::endl;
    };
    std::string s("12123");
    auto future = parser.parseAsync(s, parse_cb);
    int res = 0;
    future.get(res);
    // parser.parse("123123", res);
    std::cout << "res: " << res << std::endl;
    return 0;
}