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
  MOLE::T("trace");
  MOLE::D("debug");
  MOLE::I("info");
  MOLE::W("warn");
  MOLE::E("error");

  auto& logger = MOLE::get_logger("main");
  logger.trace("trace");
  logger.debug("debug");
  logger.info("info");
  logger.warn("warn");
  logger.error("error");

  MOLE_TRACE("trace");
  MOLE_DEBUG("debug");
  MOLE_INFO("info");
  MOLE_WARN("warn");
  MOLE_ERROR("error");

  MOLE::TracerUs tracer("test", "tracer");
  tracer.with("key","value");
  tracer.step("send request");
  tracer.step("recv response");
  tracer.step("process output");
  tracer.done("work done");

  MOLE_FATAL("1");
}
