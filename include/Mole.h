/**
  ******************************************************************************
  * @file           : Mole.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2025/3/28
  ******************************************************************************
  */

#ifndef MOLE_MOLE_H
#define MOLE_MOLE_H

#include <fmt/format.h>
#include <blockingconcurrentqueue.h>
#include <functional>
#include <vector>
#include <unordered_map>
#include <utility>

namespace mole {
namespace _internal {

#if defined(WIN32) || defined(_WIN32)
#define FILENAME(x) (strrchr(x,'\\') ? strrchr(x,'\\')+1 : x)
#ifdef MOLE_EXPORTS
#define MOLE_API __declspec(dllexport)
#else
#define MOLE_API __declspec(dllimport)
#endif // MOLE_EXPORTS
#else  // NOT WIN32 || _WIN32 end
#define MOLE_API
#define FILENAME(x) (strrchr(x, '/') ? strrchr(x, '/')+1 : x)
#endif // WIN32 || _WIN32

#define MOLE ::mole::_internal::Mole

  template<class Accuracy>
  class Tracer;

  class MOLE_API Mole {
    enum class Option : uint8_t {
      mNONE, mLEVEL, mPATH, mENABLE, mDISABLE
    };
   public:
    class Logger;
    template<class Accuracy>
    using Tracer = Tracer<Accuracy>;
    using TracerUs = Tracer<std::chrono::microseconds>;
    using TracerMs = Tracer<std::chrono::milliseconds>;
    using TracerS = Tracer<std::chrono::seconds>;
    using TracerM = Tracer<std::chrono::minutes>;
    using TracerH = Tracer<std::chrono::hours>;

    enum class Level : uint8_t {
      mTRACE, mDEBUG, mINFO, mWARN, mERROR, mFATAL
    };
   private:
    struct Entry {
      using time_point = std::chrono::system_clock::time_point;

      Level level;
      time_point time;
      const char* file;
      unsigned int line;
      std::string content;
      std::thread::id thread_id;

      Option option;
      Logger* logger;

      explicit Entry(
          Logger* logger = nullptr,
          Option option = Option::mNONE,
          Level level = MOLE::Level::mTRACE,
          std::string&& content = {},
          time_point time = {},
          const char* file = nullptr,
          unsigned int line = 0,
          std::thread::id thread_id = {}
      );
    };
   public:
    using Channel = moodycamel::BlockingConcurrentQueue<Entry>;
    class Logger {
     public:
      ~Logger();
      template<class ...Args>
      void trace(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mTRACE, format, args...); }
      template<class ...Args>
      void debug(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mDEBUG, format, args...); }
      template<class ...Args>
      void info(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mINFO, format, args...); }
      template<class ...Args>
      void warn(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mWARN, format, args...); }
      template<class ...Args>
      void error(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mERROR, format, args...); }
      template<class ...Args>
      void fatal(fmt::format_string<Args...> format,Args&& ...args) { this->log(MOLE::Level::mFATAL, format, args...); exit(1); }

      void set_log_path(const std::string& path);
      void set_log_level(Level level);
      void enable();
      void disable();

      void _log(Option option = MOLE::Option::mNONE,
                Level level = MOLE::Level::mTRACE,
                std::string content = {},
                std::chrono::system_clock::time_point time = {},
                const char* file = nullptr,
                unsigned int line = 0,
                std::thread::id thread_id = {});

      template<class ...Args>
      void log(MOLE::Level level,fmt::format_string<Args...> format,Args ...args) {
        if(filter > level) return;
        _log(Option::mNONE, level, fmt::format(format, args...), std::chrono::system_clock::now(), nullptr, 0, std::this_thread::get_id());
      }
     private:
      friend class Mole;
      explicit Logger(MOLE::Channel& channel, std::string name = "");
      void process_entry(Entry&);

      std::atomic<bool> is_enable{true};
      std::string name;
      FILE* fp = stdout;
      Channel& chan;
      volatile Level filter = Level::mTRACE;
    };

    Mole& operator=(const Mole&) = delete;
    Mole(const Mole&) = delete;

    static Logger& get_logger(const std::string& = {}) noexcept;
    template<class ...Args>
    static inline void T(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mTRACE, format, args...); }
    template<class ...Args>
    static inline void D(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mDEBUG, format, args...); }
    template<class ...Args>
    static inline void I(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mINFO, format, args...); }
    template<class ...Args>
    static inline void W(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mWARN, format, args...); }
    template<class ...Args>
    static inline void E(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mERROR, format, args...); }
    template<class ...Args>
    static inline void F(fmt::format_string<Args&&...> format,Args ...args) { get_logger().log(MOLE::Level::mFATAL, format, args...); exit(1); }

    static void set_log_path(const std::string& path);
    static void set_log_level(Level level);
    static void enable();
    static void disable();

   private:
    explicit Mole() = default;
    ~Mole();
    std::shared_ptr<Logger> make_shared_logger(std::string name);
    static void process_loop(Mole*);

    Channel chan;
    std::thread process_thread{process_loop, this};
    std::atomic<bool> stop{false};
    std::unordered_map<std::string, std::shared_ptr<Logger>> loggers;
    std::mutex logger_mtx;
  };

#define MOLE_LOG(level,content,...) do{ \
  MOLE::get_logger()._log({}, level,::fmt::format(content,##__VA_ARGS__),std::chrono::system_clock::now(), FILENAME(__FILE__), __LINE__,std::this_thread::get_id());\
}while(0)
#define MOLE_TRACE(content,...) MOLE_LOG(MOLE::Level::mTRACE, content, ##__VA_ARGS__)
#define MOLE_DEBUG(content,...) MOLE_LOG(MOLE::Level::mDEBUG, content, ##__VA_ARGS__)
#define MOLE_INFO(content,...)  MOLE_LOG(MOLE::Level::mINFO, content, ##__VA_ARGS__)
#define MOLE_WARN(content,...)  MOLE_LOG(MOLE::Level::mWARN, content, ##__VA_ARGS__)
#define MOLE_ERROR(content,...) MOLE_LOG(MOLE::Level::mERROR, content, ##__VA_ARGS__)
#define MOLE_FATAL(content,...) do {                    \
  MOLE_LOG(MOLE::Level::mFATAL, content, ##__VA_ARGS__); \
  exit(1);                                               \
}while(0)

const std::string top_left = "┌", top_right = "┐", bottomLeft = "└", bottom_right = "┘";
const std::string horizontal = "─", vertical = "│";
const std::string top_joint = "┬", bottom_joint = "┴", middle_joint = "┼";
const std::string left_joint = "├", right_joint = "┤";

template<class Accuracy = std::chrono::milliseconds>
class Tracer {
 public:
  explicit Tracer(const std::string& name,const std::string& logger_name = "") : logger(MOLE::get_logger(logger_name)) {
    data.emplace_back(std::vector<std::string>{fmt::format("Tracer:{}",name), "Step", "Elapsed"});
  }
  ~Tracer() { done(); }
  explicit Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;
  void reset() {
    data.clear();
    last = std::chrono::system_clock::time_point{};
    start = std::chrono::system_clock::now();
  }
  void step(const std::string& stage) {
    if (start == std::chrono::system_clock::time_point{}) reset();

    if (last <= start) {
      last = start;
    }
    auto now = std::chrono::system_clock::now();
    auto dura = now - last;
    last = now;
    data.emplace_back(std::vector<std::string>{fmt::format("{}",data.size()), stage, fmt::format("{} {}", std::chrono::duration_cast<Accuracy>(dura).count(), unit())});
  }
  void done(const std::string& stage = "") {
    if (start == std::chrono::system_clock::time_point{}) return;
    if(!stage.empty()) step(stage);
    data.emplace_back(std::vector<std::string>{"-", "Total", fmt::format("{} {}", std::chrono::duration_cast<Accuracy>(std::chrono::system_clock::now() - start).count(), unit())});
    if(data.size() <= 2) return;
    logger.trace(labels + gen_table());
    start = {};
  }
  Tracer& with(const std::string& key, const std::string& value) {
    labels += (key + "=" + value);
    return *this;
  }
 private:
  std::string unit() {
    if (std::is_same<Accuracy, std::chrono::nanoseconds>::value) { return "ns"; }
    else if(std::is_same<Accuracy, std::chrono::microseconds>::value) { return "us"; }
    else if(std::is_same<Accuracy, std::chrono::milliseconds>::value) { return "ms"; }
    else if(std::is_same<Accuracy, std::chrono::seconds>::value) { return "s"; }
    else if(std::is_same<Accuracy, std::chrono::minutes>::value) { return "m"; }
    else if(std::is_same<Accuracy, std::chrono::hours>::value) { return "h"; }
    return "";
  }
  std::string gen_table() {
    std::vector<int> column_widths(data[0].size(), 0);
    for (const auto& row : data) {
      for (size_t i = 0; i < row.size(); ++i) {
        column_widths[i] = column_widths[i] > static_cast<int>(row[i].size()) ? column_widths[i] : static_cast<int>(row[i].size());
      }
    }
    std::string stream = "\n";

    auto printLine = [&](const std::string& left, const std::string& joint, const std::string& right) -> std::string {
      std::string line = left;
      for (size_t i = 0; i < column_widths.size(); ++i) {
        line += fmt::format("{:─^{}}", horizontal, column_widths[i] + 2);
        if (i < column_widths.size() - 1) line += joint;
      }
      line += right;
      return fmt::format("{}", line);
    };

    stream += printLine(top_left, top_joint, top_right) + "\n";

    for (size_t i = 0; i < data.size(); ++i) {
      std::string row = vertical;
      for (size_t j = 0; j < data[i].size(); ++j) {
        row += fmt::format(" {:^{}} {}", data[i][j], column_widths[j], vertical);
      }
      stream += fmt::format("{}\n", row);

      if (i != data.size() -1) {
        stream += printLine(left_joint, middle_joint, right_joint) + "\n";
      } else {
        stream +=  printLine(bottomLeft, bottom_joint, bottom_right);
      }
    }
    return stream;
  }
  Mole::Logger& logger;
  std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point last = {};
  std::vector<std::vector<std::string>> data;
  std::string labels;
};

} // _internal namespace
} // mole namespace



#endif //MOLE_MOLE_H
