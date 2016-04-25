#include "diceroller.h"

#include <random>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>

#include <glog/logging.h>

#include "util/calc.h"

namespace {
	std::shared_ptr<std::mt19937_64> rng;
}

class DiceRoll
{
public:
	DiceRoll(int dice, int numberOfRolls, std::mt19937_64 &generator)
		: _dice(dice)
		, _numberOfRolls(numberOfRolls)
	{
		std::uniform_int_distribution<int> dis(1, _dice);

		_resultDescription = std::to_string(_numberOfRolls) + "d" + std::to_string(_dice) + " {";
		for (int i = 0; i < _numberOfRolls; ++i)
		{
			auto roll = dis(generator);
			_result += roll;
			_resultDescription.append(i == 0 ? std::to_string(roll) : " " + std::to_string(roll));
		}

		_resultDescription.append(_numberOfRolls > 1 ? " : " + std::to_string(_result) + "}" : "}");
	}

	const std::string &GetDescription() const
	{
		return _resultDescription;
	}

	long GetResult() const
	{
		return _result;
	}

private:
	int _dice = 0;
	int _numberOfRolls = 0;

	long _result = 0;
	std::string _resultDescription;
};

std::vector<std::string> GetDiceTokens(std::string rawInput)
{
	std::vector<std::string> output;

	if (rawInput.empty())
		return output;

	if (rawInput.at(0) != '.')
		return output;

	rawInput.erase(std::remove_if(rawInput.begin(), rawInput.end(), [](char c){ return std::isspace(static_cast<unsigned char>(c)); }),rawInput.end());

	std::string token;
	size_t prevPos = 1;
	size_t pos = 1;
	bool prevOp = true;
	while (pos != rawInput.npos && pos < rawInput.size())
	{
		pos = rawInput.find_first_of("+-*/\\^%()", prevPos);

		if (pos == rawInput.npos)
			break;

		if (rawInput.at(pos) == '-')
		{
			// overly complex workaround for leading unary minus
			if ((pos != 1 && prevOp) || (pos == 1 && rawInput.size() > 1 && rawInput.at(2) != '('))
				pos = rawInput.find_first_of("+-*/\\^%()", prevPos + 1);
			else if (pos == 1 && rawInput.size() > 1 && rawInput.at(2) == '(')
				output.push_back("0");
		}

		if (pos == prevPos)
		{
			output.push_back(std::string(1, rawInput.at(pos)));
			prevPos = pos + 1;
			prevOp = true;
		}
		else if (pos != rawInput.npos)
		{
			prevOp = false;
			output.push_back(rawInput.substr(prevPos, pos - prevPos));
			prevPos = pos;
		}
	}
	token = rawInput.substr(prevPos, rawInput.size() - prevPos);
	output.emplace_back(token);

	return output;
}

DiceRoller::DiceRoller(LemonBot *bot)
	: LemonHandler("dice", bot)
{
	ResetRNG();
}

LemonHandler::ProcessingResult DiceRoller::HandleMessage(const std::string &from, const std::string &body)
{
	auto diceTokens = GetDiceTokens(body);

	if (diceTokens.empty())
		return ProcessingResult::KeepGoing;

	std::string resultDescription;
	double result = 0;

	ShuntingYard sy;
	for (const auto &token : diceTokens)
	{
		auto dpos = token.find('d');
		if (dpos == token.npos)
		{
			resultDescription += token + " ";
			if (!sy.PushToken(token))
			{
				LOG(WARNING) << "Failed to push token to RPN: " << token;
				return ProcessingResult::KeepGoing;
			}
		} else {
			long rollResult = 0;
			try {
				auto rolls = std::stoi(token.substr(0, dpos));
				auto sides = std::stoi(token.substr(dpos + 1));

				if (rolls < 1 || rolls > 100 || sides < 1 || sides > 10000)
				{
					LOG(WARNING) << "Invalid dice: " << token;
					return ProcessingResult::KeepGoing;
				}

				DiceRoll d(sides, rolls, *rng.get());
				rollResult = d.GetResult();
				resultDescription += d.GetDescription() + " ";
			} catch (std::exception &e) {
				LOG(WARNING) << "Failed to parse dice token " << token << ": " << e.what();
			}

			if (rollResult == 0)
			{
				LOG(WARNING) << "Dice roll returned 0, invalid dice? " << token;
				return ProcessingResult::KeepGoing;
			}

			if (!sy.PushToken(std::to_string(rollResult)))
			{
				LOG(WARNING) << "Failed to push token to RPN: " << rollResult;
				return ProcessingResult::KeepGoing;
			}
		}
	}

	if (!sy.Finalize())
	{
		LOG(WARNING) << "Failed to finalize RPN, parenthesis mismatch? Lists: " << sy.GetDebugDescription();
		return ProcessingResult::KeepGoing;
	}

	if (!EvaluateRPN(sy.GetRPN(), result))
	{
		LOG(WARNING) << "Failed to evaluate RPN. Lists: " << sy.GetDebugDescription();
		return ProcessingResult::KeepGoing;
	}

	if (diceTokens.size() > 1)
		resultDescription.append("= " + std::to_string(result));

	SendMessage(from + ": " + resultDescription);

	return ProcessingResult::StopProcessing;
}

const std::string DiceRoller::GetHelp() const
{
	return "Start your message with . (dot) and write an expression using integer numbers, dice"
		   " in format XdY, parenthesis or operators +-*/\\%^";
}

void DiceRoller::ResetRNG(int seed)
{
	rng.reset(new std::mt19937_64(seed !=0 ? seed : std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

#include <set>

class DiceTestBot : public LemonBot
{
public:
	void SendMessage(const std::string &text)
	{
		if (text.find('=') > text.find('}') && text.find('=') < text.npos)
			_lastResult = std::stoi(text.substr(text.find_last_of('=') + 1));
		else
			_lastResult = std::stoi(text.substr(text.find('{') + 1, text.find('}') - text.find('{') - 1));
	}

	int _lastResult = 0;
};

TEST(DiceTest, TokenizerTest)
{
	auto test1 = GetDiceTokens(".1d20");
	EXPECT_EQ("1d20", test1.at(0));

	auto test2 = GetDiceTokens(".1d5+2d8");
	EXPECT_EQ("1d5", test2.at(0));
	EXPECT_EQ("+", test2.at(1));
	EXPECT_EQ("2d8", test2.at(2));

	auto test3 = GetDiceTokens(".2d2 +  3d1-  5");
	EXPECT_EQ("2d2", test3.at(0));
	EXPECT_EQ("+", test3.at(1));
	EXPECT_EQ("3d1", test3.at(2));
	EXPECT_EQ("-", test3.at(3));
	EXPECT_EQ("5", test3.at(4));

	auto test4 = GetDiceTokens(".4d3 -2d1+ 77d55 -  10");
	EXPECT_EQ("4d3", test4.at(0));
	EXPECT_EQ("-", test4.at(1));
	EXPECT_EQ("2d1", test4.at(2));
	EXPECT_EQ("+", test4.at(3));
	EXPECT_EQ("77d55", test4.at(4));
	EXPECT_EQ("-", test4.at(5));
	EXPECT_EQ("10", test4.at(6));

	auto test5 = GetDiceTokens(".-5d2-4d1");
	EXPECT_EQ("-5d2", test5.at(0));
	EXPECT_EQ("-", test5.at(1));
	EXPECT_EQ("4d1", test5.at(2));
}

TEST(DiceTest, InvalidTokens)
{
	auto test1 = GetDiceTokens("not a token");
	EXPECT_TRUE(test1.empty());

	auto test2 = GetDiceTokens("");
	EXPECT_TRUE(test2.empty());

	auto test3 = GetDiceTokens(".invalid token");
	EXPECT_LT(0, test3.size());
}

TEST(DiceTest, DiceRolls)
{
	DiceTestBot testbot;
	DiceRoller r(&testbot);

	r.HandleMessage("test", ".1d1");
	EXPECT_EQ(1, testbot._lastResult);

	r.HandleMessage("test", ".1d1 + 7 - 12 + 3d1");
	EXPECT_EQ(1+7-12+3, testbot._lastResult);

	r.ResetRNG(1);
	r.HandleMessage("test", ".1d6");
	EXPECT_EQ(1, testbot._lastResult);

	r.HandleMessage("test", ".1d30");
	EXPECT_EQ(5, testbot._lastResult);

	r.HandleMessage("test", ".10d10 - 5d8 + 100d3");
	EXPECT_EQ(228, testbot._lastResult);
}

#endif // LCOV_EXCL_STOP
