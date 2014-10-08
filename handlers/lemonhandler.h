#ifndef LEMONHANDLER_H
#define LEMONHANDLER_H

#include <string>

class LemonBot
{
public:
	virtual void SendMessage(const std::string &text) const = 0;
	virtual ~LemonBot() {}
};

class LemonHandler
{
public:
	LemonHandler(LemonBot *bot);
	virtual ~LemonHandler();

public:
	virtual bool HandleMessage(const std::string &from, const std::string &body) = 0;
	virtual const std::string GetVersion() const = 0;

protected:
	void SendMessage(const std::string &text);
	LemonBot *m_Bot;
};

#endif // LEMONHANDLER_H
