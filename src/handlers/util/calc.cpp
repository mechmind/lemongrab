#include "calc.h"

#include <iostream>
#include <cmath>

int GetPrecedence(const std::string &token);
bool LeftAssoc(const std::string &token);

bool ShuntingYard::PushToken(const std::string &token)
{
	if (token == "(")
	{
		_operators.push_front(token);
		return true;
	}

	if (token == ")")
	{
		while (!_operators.empty() && _operators.front() != "(")
		{
			_rpn.push_back(_operators.front());
			_operators.pop_front();
		}

		if (_operators.empty())
			return false;
		else
		{
			_operators.pop_front();
			return true;
		}
	}

	auto p = GetPrecedence(token);

	if (p == 0)
	{
		_rpn.push_back(token);
		return true;
	}
	else
	{
		while (!_operators.empty() &&
			   ((LeftAssoc(token) && GetPrecedence(_operators.front()) >= p)
				|| (!LeftAssoc(token) && GetPrecedence(_operators.front()) > p)))
		{
			_rpn.push_back(_operators.front());
			_operators.pop_front();
		}
		_operators.push_front(token);
		return true;
	}
}

bool ShuntingYard::Finalize()
{
	while (!_operators.empty() && _operators.back() != "(")
	{
		_rpn.push_back(_operators.front());
		_operators.pop_front();
	}
	return _operators.empty();
}

const std::list<std::string> &ShuntingYard::GetRPN()
{
	return _rpn;
}

bool LeftAssoc(const std::string &token)
{
	return token != "^";
}

int GetPrecedence(const std::string &token)
{
	if (token == "^")
		return 10;

	if (token == "*" || token == "/" || token == "\\" || token == "%")
		return 9;

	if (token == "+" || token == "-")
		return 8;

	return 0;
}

bool EvaluateRPN(const std::list<std::string> &rpn, double &output)
{
	std::list<double> values;

	for (const auto &token : rpn)
	{
		if (GetPrecedence(token) == 0)
		{
			try {
				auto value = std::stod(token);
				values.push_back(value);
			} catch (std::exception &e) {
				std::cout << "Something went wrong: " << e.what() << std::endl;
			}
		} else {
			if (values.size() < 2)
				return false;

			auto arg2 = values.back();
			values.pop_back();
			auto arg1 = values.back();
			values.pop_back();

			double result = 0;
			if (token == "+")
			{
				result = arg1 + arg2;
			} else if (token == "-") {
				result = arg1 - arg2;
			} else if (token == "*") {
				result = arg1 * arg2;
			} else if (token == "/") {
				result = arg1 / arg2;
			} else if (token == "\\") {
				result = static_cast<long long>(arg1 / arg2);
			} else if (token == "%") {
				result = std::remainder(arg1, arg2);
			} else if (token == "^") {
				result = std::pow(arg1, arg2);
			} else {
				return false;
			}
			values.push_back(result);
		}
	}

	if (values.size() != 1)
		return false;
	else
	{
		output = *values.begin();
		return true;
	}
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

TEST(Evaluator, ShuntingYardTest)
{
	ShuntingYard s;
	std::list<std::string> input = { "3", "+", "4", "*", "2", "/", "(", "1", "-", "5", ")", "^", "2", "^", "3" };
	std::list<std::string> output = { "3", "4", "2", "*", "1", "5", "-", "2", "3", "^", "^", "/", "+" };

	for (auto &i : input)
		EXPECT_TRUE(s.PushToken(i));

	ASSERT_TRUE(s.Finalize());
	auto res = s.GetRPN();

	ASSERT_EQ(output.size(), res.size());

	auto o = output.begin();
	auto r = res.begin();
	for (; o != output.begin(); ++o, ++r)
		EXPECT_EQ(*o, *r);
}

#endif // LCOV_EXCL_STOP
