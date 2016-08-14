#pragma once

#include "lemonhandler.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class DiceRoller : public LemonHandler
{
public:
	DiceRoller(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

private:
	void ResetRNG(int seed = 0);

#ifdef _BUILD_TESTS
	FRIEND_TEST(DiceTest, DiceRolls);
#endif
};
