#include "diceroller.h"

#include <random>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>

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
	for (size_t pos = 1; pos < rawInput.size(); pos++)
	{
		if (rawInput.at(pos) == '+' || rawInput.at(pos) == '-')
		{
			token = rawInput.substr(prevPos, pos - prevPos);
			output.emplace_back(token);

			prevPos = rawInput.at(pos) == '-' ? pos : pos + 1;
		}
	}
	token = rawInput.substr(prevPos, rawInput.size() - prevPos);
	output.emplace_back(token);

	return output;
}

bool ParseSingleToken(const std::string &token, std::string &resultDescription, long &result)
{
	if (token.empty())
		return false;

	auto negative = token.at(0) == '-';

	if (!negative && !resultDescription.empty())
		resultDescription.append(" + ");
	else if (negative)
		resultDescription.append(" - ");

	auto diceSeparatorPosition = token.find('d');

	if (diceSeparatorPosition == token.npos)
	{
		try {
			auto simpleNumber = std::stol(token);
			result = result + simpleNumber;
			resultDescription.append(negative ? std::to_string(-1*simpleNumber) : std::to_string(simpleNumber));
		} catch (...) {
			return false;
		}
	} else {
		try {
			auto numberOfRolls = diceSeparatorPosition > 0 ? std::stoi(token.substr(negative ? 1 : 0, diceSeparatorPosition)) : 1;
			auto dice = std::stoi(token.substr(diceSeparatorPosition + 1));

			if (numberOfRolls > 100)
				return false;

			DiceRoll roll(dice, numberOfRolls, *rng.get());

			result = negative ? result - roll.GetResult() : result + roll.GetResult();
			resultDescription.append(roll.GetDescription());
		} catch (...) {
			return false;
		}
	}
	return true;
}

DiceRoller::DiceRoller(LemonBot *bot)
	: LemonHandler("dice", bot)
{
	ResetRNG();
}

bool DiceRoller::HandleMessage(const std::string &from, const std::string &body)
{
	auto diceTokens = GetDiceTokens(body);

	if (diceTokens.empty())
		return false;

	std::string resultDescription;
	long result = 0;
	bool success = true;
	for (auto token : diceTokens)
	{
		if (!ParseSingleToken(token, resultDescription, result))
		{
			// resultDescription = "Failed to parse token: " + token;
			success = false;
			break;
		}
	}

	if (success && diceTokens.size() > 1)
		resultDescription.append(" = " + std::to_string(result));

	if (success)
		SendMessage(from + ": " + resultDescription);

	return true;
}

const std::string DiceRoller::GetVersion() const
{
	return "0.2";
}

const std::string DiceRoller::GetHelp() const
{
	return "Start your message with . (dot) and write an expression using integer numbers, dice"
		   " in format XdY, and operators + or -";
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
	EXPECT_EQ("2d8", test2.at(1));

	auto test3 = GetDiceTokens(".2d2 +  3d1-  5");
	EXPECT_EQ("2d2", test3.at(0));
	EXPECT_EQ("3d1", test3.at(1));
	EXPECT_EQ("-5", test3.at(2));

	auto test4 = GetDiceTokens(".4d3 -2d1+ 77d55 -  10");
	EXPECT_EQ("4d3", test4.at(0));
	EXPECT_EQ("-2d1", test4.at(1));
	EXPECT_EQ("77d55", test4.at(2));
	EXPECT_EQ("-10", test4.at(3));
}

TEST(DiceTest, NonDiceTokens)
{
	std::string resultS;
	long result = 0;
	ParseSingleToken("5", resultS, result);
	EXPECT_EQ(5, result);
	EXPECT_EQ("5", resultS);

	ParseSingleToken("4", resultS, result);
	EXPECT_EQ(9, result);
	EXPECT_EQ("5 + 4", resultS);

	ParseSingleToken("-3", resultS, result);
	EXPECT_EQ(6, result);
	EXPECT_EQ("5 + 4 - 3", resultS);
}

TEST(DiceTest, InvalidTokens)
{
	auto test1 = GetDiceTokens("not a token");
	EXPECT_TRUE(test1.empty());

	auto test2 = GetDiceTokens("");
	EXPECT_TRUE(test2.empty());

	auto test3 = GetDiceTokens(".invalid token");
	std::string desc;
	long int result = 0;
	EXPECT_LT(0, test3.size());
	EXPECT_FALSE(ParseSingleToken(*test3.begin(), desc, result));
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
