#pragma once

#include <string>
#include <list>

void initLocale();

std::string toLower(const std::string &input);

std::list<std::string> tokenize(const std::string &input, char separator);
