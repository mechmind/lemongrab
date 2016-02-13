#pragma once

#include <string>

class LemonBot
{
public:
	virtual void SendMessage(const std::string &text) const = 0;
	virtual const std::string GetRawConfigValue(const std::string &name) const = 0;
	virtual ~LemonBot() {}
};

class LemonHandler
{
public:
	/**
	 * Delegate this constructor in your message handler
	 */
	LemonHandler(LemonBot *bot);
	virtual ~LemonHandler();

public:
	/**
	 * @brief Receives and handles MUC messages
	 * @param from Sender nickname
	 * @param body Message body
	 * @return True if message shoud not be passed to another handler
	 */
	virtual bool HandleMessage(const std::string &from, const std::string &body) = 0;
	virtual bool HandlePresence(const std::string &from, const std::string &jid, bool connected) { return false; }

	/**
	 * @brief Get module version
	 * @return Version string (e.g. "Dice 0.1")
	 */
	virtual const std::string GetVersion() const = 0;

protected:
	/**
	 * @brief Send reply to MUC
	 * @param text
	 */
	void SendMessage(const std::string &text);
	const std::string GetRawConfigValue(const std::string &name) const;
	LemonBot *m_Bot;
};
