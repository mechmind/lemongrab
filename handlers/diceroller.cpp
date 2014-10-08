#include "diceroller.h"

#include <sstream>

static void ParseDice(const std::string &text, int &X, int &Y)
{
	auto dot = text.find_first_of(".");
	auto delim = text.find_first_of("d");

	bool schar = (dot == 0 && delim != text.npos);

	if (schar)
	{
		auto sX = text.substr(1, delim - 1);
		auto sY = text.substr(delim + 1, text.npos);

		X = std::atoi(sX.c_str());
		Y = std::atoi(sY.c_str());
	}
}

bool DiceRoller::HandleMessage(const std::string &from, const std::string &body)
{
	// TODO proper handling of utf8 strings
	int X(-1), Y(-1);
	ParseDice(body, X, Y);

	if (X < 1 || Y < 1)
		return false;

	if (X > 50 || Y > 1000)
	{
		std::stringstream reply;
		reply << from << ": Now try that again";
		SendMessage(reply.str());
		return true;
	}

	std::uniform_int_distribution<int> dis(1, Y);

	std::stringstream reply;
	reply << from << ": { ";
	for (int i = 0; i < X; ++i)
	{
		reply << dis(m_gen) << " ";
	}
	reply << "}";

	SendMessage(reply.str());
	return true;
}

const std::string DiceRoller::GetVersion() const
{
	return "Dice: 0.1";
}
