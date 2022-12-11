#ifndef _SERIAL_DEBUG_H_
#define _SERIAL_DEBUG_H_

enum class LOG_LEVEL
{
	INFO,
	WARN,
	ERROR,
	DEBUG
};

class SerialDebug
{
public:

	static void log(LOG_LEVEL level, String msg);
};

void SerialDebug::log(LOG_LEVEL level, String msg)
{
#ifdef SERIAL_DEBUG

	String msg_keyword;

	switch (level)
	{
	case LOG_LEVEL::INFO:
		msg_keyword += "INFO: ";
		break;
	case LOG_LEVEL::WARN:
		msg_keyword += "WARNING: ";
		break;
	case LOG_LEVEL::ERROR:
		msg_keyword += "ERROR: ";
		break;
	case LOG_LEVEL::DEBUG:
		msg_keyword += "DEBUG: ";
		break;
	default:
		return;
	}

	msg_keyword += msg;

	Serial.println(msg_keyword);

#endif
}

#endif
