#include "datastorage.h"

DataStorage::DataStorage()
	: m_Conference("dfwk@conference.jabber.ru/LEMONGRAB") // FIXME
{

}

bool DataStorage::ReadFromFile(std::string path)
{
	return false;
}

const std::string &DataStorage::GetConference() const
{
	return m_Conference;
}

const Credentials &DataStorage::GetCredentials() const
{
	return m_Credentials;
}
