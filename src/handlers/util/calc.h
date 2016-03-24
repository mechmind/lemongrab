#pragma once

#include <list>
#include <string>

class ShuntingYard
{
public:
	bool PushToken(const std::string &token);
	bool Finalize();
	const std::list<std::string> &GetRPN();

private:
	std::list<std::string> _operators;
	std::list<std::string> _rpn;
};

bool EvaluateRPN(const std::list<std::string> &rpn, double &output);
