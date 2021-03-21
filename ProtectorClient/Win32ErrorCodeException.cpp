#include "Win32ErrorCodeException.h"

#include <vector>
#include <sstream>

#include <atlstr.h>

Win32ErrorCodeException::Win32ErrorCodeException(const std::string& errorMessage)
	: std::runtime_error(errorMessage), m_errorCode(0)
{
	this->m_errorCode = GetLastError();
	this->m_winErrorMessage = this->getLastErrorMessage();
}

const char* Win32ErrorCodeException::what() const
{
	std::stringstream messageStream;

	messageStream << "\n\tWindows last error code: 0x";
	messageStream << std::hex << this->m_errorCode;
	messageStream << "\n\tWindows error message: ";
	messageStream << this->m_winErrorMessage;

	return std::string(messageStream.str()).c_str();
	//return messageStream.str().c_str();
}


DWORD Win32ErrorCodeException::getErrorCode() const
{
	return this->m_errorCode;
}

std::string Win32ErrorCodeException::getWinErrorMessage() const
{
	return this->m_winErrorMessage;
}

std::string Win32ErrorCodeException::getLastErrorMessage() const
{
	const DWORD errorCode = GetLastError();
	if (0 == errorCode)
	{
		return "";
	}

	const int MESSAGE_SIZE = 512;
	std::vector<WCHAR> message(MESSAGE_SIZE);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode,
		0, &message[0], MESSAGE_SIZE, nullptr);

	return std::string(CW2A(message.data()));
}

std::string Win32ErrorCodeException::init(const std::string& errorMessage)
{
	std::ostringstream messageStream;

	messageStream << "\n\tWindows last error code: 0x";
	messageStream << std::hex << GetLastError();
	messageStream << "\n\tWindows error message: ";
	messageStream << this->getLastErrorMessage();

	return std::string(messageStream.str()).c_str();
	//return messageStream.str().c_str();
	//return messageStream.str();
}
