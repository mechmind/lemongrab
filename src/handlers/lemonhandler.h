#pragma once

#include <string>
#include <list>

#include "../xmpphandler.h" // FIXME we need chatmessage only

class LemonBot
{
public:
	virtual void SendMessage(const std::string &text) = 0;
	virtual std::string GetRawConfigValue(const std::string &name) const { return ""; }
	virtual std::string GetNickByJid(const std::string &jid)  const { return ""; }
	virtual std::string GetJidByNick(const std::string &nick) const { return ""; }
	virtual ~LemonBot() {}
};

class LemonHandler
{
public:
	/**
	 * Delegate this constructor in your message handler
	 */
	LemonHandler(const std::string &moduleName, LemonBot *bot);
	virtual ~LemonHandler();

public:
	enum class ProcessingResult {
		KeepGoing,
		StopProcessing,
	};

	/**
	 * @brief Called once when module is enabled
	 * @return True if init was successful
	 */
	virtual bool Init() { return true; }

	/**
	 * @brief Receives and handles MUC messages
	 * @param from Sender nickname
	 * @param body Message body
	 * @return True if message shoud not be passed to another handler
	 */
	virtual ProcessingResult HandleMessage(const ChatMessage &msg) = 0;
	virtual void HandlePresence(const std::string &from, const std::string &jid, bool connected) { }

	/**
	 * @brief Get help string
	 * @return List of module commands and their description
	 */
	virtual const std::string GetHelp() const;

	const std::string &GetName() const;

protected:
	/**
	 * @brief Send reply to MUC
	 * @param text
	 */
	void SendMessage(const std::string &text);
	const std::string GetRawConfigValue(const std::string &name) const;
	const std::list<std::int64_t> GetIntList(const std::string &name) const;
	std::string _moduleName;
	LemonBot *_botPtr;
};
