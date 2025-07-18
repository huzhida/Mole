/**
  ******************************************************************************
  * @file           : Mole.hpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2025/3/28
  ******************************************************************************
  */

#define _CRT_SECURE_NO_WARNINGS
#define MOLE_EXPORTS

#include <memory>
#include <utility>
#include <vector>
#include <sstream>
#include <fmt/color.h>
#include <iomanip>
#include <csignal>

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#endif

#include "Mole.h"

mole::_internal::Mole::Entry::Entry(Logger* logger, Option option, mole::_internal::Mole::Level level,std::string &&content,mole::_internal::Mole::Entry::time_point time,
                                    const char *file,unsigned int line,std::thread::id thread_id)
:level(level), time(time), file(file), line(line), content(std::move(content)), thread_id(thread_id), option(option), logger(logger){}
void mole::_internal::Mole::Logger::_log(Option option, Level level,std::string content,std::chrono::system_clock::time_point time,
                                const char* file,unsigned int line,std::thread::id thread_id) {
  thread_local std::unique_ptr<moodycamel::ProducerToken> token;
  if (!token) {
    token.reset(new moodycamel::ProducerToken(chan));
  }
  chan.enqueue(*token, Entry{this, option, level,std::move(content),time,file,line,thread_id});
}

mole::_internal::Mole::Logger &mole::_internal::Mole::get_logger(const std::string& name) noexcept {
  static Mole m;

  std::lock_guard<std::mutex> guard(m.logger_mtx);
  auto iter = m.loggers.end();
  if((iter = m.loggers.find(name)) == m.loggers.end()) {
    auto res = m.loggers.emplace(name,m.make_shared_logger(name));
    iter = res.first;
  }
  return *iter->second;
}
void mole::_internal::Mole::set_log_path(const std::string &path) {
  get_logger().set_log_path(path);
}
void mole::_internal::Mole::set_log_level(mole::_internal::Mole::Level level) {
  get_logger().set_log_level(level);
}
void mole::_internal::Mole::enable() {
  get_logger().enable();
}
void mole::_internal::Mole::disable() {
  get_logger().disable();
}

std::string time_to_date(std::chrono::system_clock::time_point time) {
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>( time.time_since_epoch()).count();
  auto t_count = std::chrono::system_clock::to_time_t(time);

  std::stringstream time_stream;
  time_stream << std::put_time(std::localtime(&t_count), "%m%d %X")
              << '.' << std::setfill('0') << std::setw(6) << microseconds % 1000000;
  return time_stream.str();
}
std::string mole::_internal::Mole::Logger::thread_id(std::thread::id id) {
  if (thread_id_map.find(id) == thread_id_map.end()) {
    ++idx;
    auto idx_str = std::to_string(idx);
    thread_id_map[id] = idx_str;
    return idx_str;
  }
  return thread_id_map[id];
}

uint64_t mole::_internal::Mole::Logger::idx = 0;
std::unordered_map<std::thread::id, std::string> mole::_internal::Mole::Logger::thread_id_map;

static const fmt::runtime_format_string<> macro_format = fmt::runtime("{}{} <tid:{}> {}[{}:{}]  {}\n");
static const fmt::runtime_format_string<> modern_cxx_format = fmt::runtime("{}{} <tid:{}> {} {}\n");
static const fmt::runtime_format_string<> macro_format_with_name = fmt::runtime("{}{} <tid:{}> ├ {} ┤ [{}:{}] {}\n");
static const fmt::runtime_format_string<> modern_cxx_format_with_name = fmt::runtime("{}{} <tid:{}> ├ {} ┤ {}\n");

void mole::_internal::Mole::Logger::process_entry(Entry& entry) {
  switch(entry.option) {
    case Option::mPATH : {
      auto new_fp = fopen(entry.content.c_str(), "ab");
      if (!new_fp) return;
      if(fp != stdout) {
        if(fclose(fp) != 0) return;
        fp = nullptr;
      }
      setvbuf(new_fp, nullptr, _IOFBF, 8192);
      fp = new_fp;
      return;
    }
    case Option::mLEVEL : {
      filter = entry.level;
      return;
    }
    case Option::mENABLE: is_enable = true; break;
    case Option::mDISABLE: is_enable = false; break;
    case Option::mNONE:break;
  }
  if(filter > entry.level) return;
#define CASE_LEVEL_STYLED(mole_level, char, style) case mole_level:level = fp == stdout ? fmt::format(style,char) : char;break;
  std::string level;
  switch(entry.level) {
    CASE_LEVEL_STYLED(Level::mTRACE, "T", fmt::fg(fmt::color::cyan))
    CASE_LEVEL_STYLED(Level::mDEBUG, "D", fmt::fg(fmt::color::light_sky_blue))
    CASE_LEVEL_STYLED(Level::mINFO, "I", fmt::fg(fmt::color::light_green))
    CASE_LEVEL_STYLED(Level::mWARN, "W", fmt::fg(fmt::color::yellow))
    CASE_LEVEL_STYLED(Level::mERROR, "E", fmt::fg(fmt::color::red))
    CASE_LEVEL_STYLED(Level::mFATAL, "F", fmt::fg(fmt::color::dark_red))
  }

  if (!entry.file) {
      fmt::print(fp,name.empty() ? modern_cxx_format : modern_cxx_format_with_name, level, time_to_date(entry.time),thread_id(entry.thread_id), name, entry.content);
    return;
  }
  fmt::print(fp,name.empty() ? macro_format : macro_format_with_name, level, time_to_date(entry.time),thread_id(entry.thread_id),name, entry.file, entry.line,  entry.content);
}

void mole::_internal::Mole::process_loop(Mole* mp) {
  sigset_t mask;
  sigfillset(&mask);
  sigprocmask(SIG_BLOCK, &mask, nullptr);
  Mole& m = *mp;
  MOLE::Entry entries[64];
  size_t num;

#if defined(_WIN32) || defined(WIN32)
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

  moodycamel::ConsumerToken token(m.chan);

  while(!m.stop) {
    if((num = m.chan.wait_dequeue_bulk_timed(token, entries, 64, std::chrono::seconds(1))) == 0) {
      continue;
    }
    for (size_t index = 0; index < num; ++index) {
      entries[index].logger->process_entry(entries[index]);
    }
  }

  while ((num = m.chan.try_dequeue_bulk(entries, 64)) != 0) {
    for (size_t index = 0; index < num; ++index) {
      entries[index].logger->process_entry(entries[index]);
    }
  }
}
mole::_internal::Mole::~Mole() {
  stop = true;
  process_thread.join();
}
std::shared_ptr<MOLE::Logger> mole::_internal::Mole::make_shared_logger(std::string name) {
  return std::shared_ptr<MOLE::Logger>(new Logger(chan,std::move(name)));
}
mole::_internal::Mole::Logger::Logger(MOLE::Channel& channel, std::string name) : name(std::move(name)), chan(channel) {}
mole::_internal::Mole::Logger::~Logger() {
  if(this->fp && this->fp != stdout) {
    fclose(fp);
    fp = nullptr;
  }
}
void mole::_internal::Mole::Logger::set_log_path(const std::string& path) {
  _log(Option::mPATH,Level::mTRACE,path);
}
void mole::_internal::Mole::Logger::set_log_level(Level level) {
  _log(Option::mLEVEL, level);
}
void mole::_internal::Mole::Logger::enable() {
  if(is_enable) return;
  _log(Option::mENABLE);
}
void mole::_internal::Mole::Logger::disable() {
  if(!is_enable) return;
  _log(Option::mDISABLE);
}




