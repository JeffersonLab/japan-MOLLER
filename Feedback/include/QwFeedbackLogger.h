#pragma once
#include <string_view>
#include <fstream>
#include <memory>
#include "QwObserver.h"
#include "QwLog.h"
struct QwFeedbackLogPayload
{
	QwLogLevel fLevel;
	std::string fPayload;
};

class QwFeedbackLogger : public VQwLogger, public Observer<QwFeedbackLogPayload>
{
	QwLogger<std::unique_ptr<std::ostream>> fSink;
public:
	void SetSink(std::string const& sink, const std::ios_base::openmode mode = std::ios::app);
	explicit operator bool() const;

public: // VQwLogger Inherited Functions
	void Write(QwLogLevel level, std::string_view log) override;
	QwLogProxy Log(QwLogLevel level, std::string_view msg = std::string_view{}) override;
public: // Observer Inherited Functions
	void Update(QwFeedbackLogPayload const& data) override;
};


