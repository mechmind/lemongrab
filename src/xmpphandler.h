#pragma once

#include <string>
#include <memory>

class MessageHandler
{
public:
	virtual void OnMessage(std::string JID, std::string name, std::string text) = 0;
};

class MessageSource
{
public:
	virtual ~MessageSource() { }

	void SetHandler(std::shared_ptr<MessageHandler> hanlder)
	{
		m_Handler = hanlder;
	}

	MessageHandler &GetMessageHandler() const
	{
		return *m_Handler.get();
	}

	virtual bool Connect(std::string JID, std::string password, std::string MUC) = 0;
	virtual bool Disconnect() = 0;
	virtual bool IsConnected() = 0;

	virtual bool SendMessage(std::string text) = 0;
	virtual bool SendMessage(std::string recepient, std::string text) = 0;

private:
	std::shared_ptr<MessageHandler> m_Handler;
};

class GlooxClient;

class XMPPHandler
		: public MessageSource
{
public:
	XMPPHandler();

	bool Connect(std::string JID, std::string password, std::string MUC);
	bool Disconnect();
	bool IsConnected();

private:
	bool m_IsConnected = false;

private:
	std::shared_ptr<GlooxClient> m_Gloox;
};
