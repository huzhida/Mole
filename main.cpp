/**
  ******************************************************************************
  * @file           : main.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2025/3/28
  ******************************************************************************
  */
#include "Mole.h"

int main() {
  MOLE::T("123{}", 4);
  MOLE::I("11111111");
  MOLE::E("2222222222");
  MOLE::I("zzzzzzzzzzzzz");


  MOLE::TracerUs tracer("default", "sglang");
  tracer.step("send request");
  tracer.step("recv response");
  tracer.step("process output");

  auto t = std::thread([]{
    auto& logger = MOLE::get_logger("worker");
    logger.info("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
  });
  auto& logger = MOLE::get_logger("sglang");
//  logger.set_log_path("../tmp.log");
  tracer.done("work done");
  logger.trace("aaaaaaaaaaaaaaaaaa");
  logger.info("bbbbbbbbbbbbvbbbbbb");
  logger.debug("zzzzzzzzzzzzzzzzzzzzzzzzzz");
  MOLE_INFO("11111");
  t.join();
  MOLE::Channel chan;
}
