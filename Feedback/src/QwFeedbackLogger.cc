#include "QwFeedbackLogger.h"

void QwFeedbackLogger::SetSink(std::string const& sink, const std::ios_base::openmode mode)
{
	fSink = std::make_unique<std::ofstream>(sink, std::ios::out | mode);
}

QwFeedbackLogger::operator bool() const
{
	return static_cast<bool>(fSink);
}

void QwFeedbackLogger::Write(QwLogLevel level, std::string_view log)
{
	// Its up to the user to make sure this is not NULL
	// But what to do if the user doesn't care? 
	// 	Throw an exception?
	// 	Use the global logger?
	if(fSink) fSink.Write(level, log);
	else gQwLogger::Instance().Write(level, log);
}

QwLogProxy QwFeedbackLogger::Log(QwLogLevel level, std::string_view msg)
{
	// Its up to the user to make sure this is not NULL
	// But what to do if the user doesn't care? 
	// 	Throw an exception?
	// 	Use the global logger?
	if(fSink) return fSink.Log(level, msg);
	else return gQwLogger::Instance().Log(level, msg);
}

void QwFeedbackLogger::Update(QwFeedbackLogPayload const& data)
{
	Log(data.fLevel, data.fPayload);
}
