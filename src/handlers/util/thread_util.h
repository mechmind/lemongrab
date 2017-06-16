#pragma once

#include <thread>
#include <string>

void nameThread(std::thread &thread, const std::string &name);
void nameThisThread(const std::string &name);
