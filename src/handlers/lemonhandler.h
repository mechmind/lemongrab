#pragma once

#include <string>
#include <list>

#include "../xmpphandler.h" // FIXME we need chatmessage only

#include "util/sqlite_db.h"

class LemonBot
{
public:
	explicit LemonBot(const std::string &storagePath)
		: _storage(initStorage(storagePath))
	{}

	virtual void SendMessage(const std::string &text) {}
	virtual void SendMessage(const std::string &text, const std::string &module_name) {
		SendMessage(text);
	}
	virtual void TunnelMessage(const ChatMessage &msg, const std::string &module_name) {}

	virtual std::string GetRawConfigValue(const std::string &name) const { return ""; }
	virtual std::string GetRawConfigValue(const std::string &table, const std::string &name) const { return ""; }
	virtual std::string GetNickByJid(const std::string &jid)  const { return ""; }
	virtual std::string GetJidByNick(const std::string &nick) const { return ""; }
	virtual std::string GetOnlineUsers() const { return ""; }
	virtual std::string GetDBPathPrefix() const { return "db/"; }
	virtual ~LemonBot() {}

	Storage _storage;
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

	/**
	 * @brief TunnelMessage
	 * @param msg
	 */
	void TunnelMessage(const ChatMessage &msg);

	const std::string GetRawConfigValue(const std::string &name) const;
	const std::string GetRawConfigValue(const std::string &table, const std::string &name) const;
	const std::list<std::int64_t> GetIntList(const std::string &name) const;
	std::string _moduleName;
	LemonBot *_botPtr;

	Storage &getStorage() {
		if (_botPtr)
			return _botPtr->_storage;
		else
		{
			static Storage storage = initStorage(":memory:");
			return storage;
		}
	}

	const Storage &getStorage() const {
		if (_botPtr)
			return _botPtr->_storage;
		else
		{
			static Storage storage = initStorage(":memory:");
			return storage;
		}
	}
};
