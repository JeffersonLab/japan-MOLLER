#include <fstream>
#include <regex>
#include <chrono>
#include <algorithm>

// Qweak Headers
#include "QwLog.h"
#include "QwOptions.h"

QwLogLevel ConvertToEnum(int thr)
{
	auto level = QwLogLevel::kAlways;
	switch(static_cast<QwLogLevel>(thr)) {
		case QwLogLevel::kError:   level = QwLogLevel::kError  ; break; 
		case QwLogLevel::kWarning: level = QwLogLevel::kWarning; break;
		case QwLogLevel::kMessage: level = QwLogLevel::kMessage; break;
		case QwLogLevel::kVerbose: level = QwLogLevel::kVerbose; break;
		case QwLogLevel::kDebug:   level = QwLogLevel::kDebug  ; break;
		default: level = QwLogLevel::kAlways; break;
	}
	return level;
}
QwColor GetLevelColor(QwLogLevel level)
{
	auto color = QwColor(Qw::kNormal);
	switch (level) {
		case QwLogLevel::kError:   color = QwColor(Qw::kRed); break;
		case QwLogLevel::kWarning: color = QwColor(Qw::kMagenta); break;
		case QwLogLevel::kDebug:   color = QwColor(Qw::kBlue); break;
		default: break;
	}
	return color;
}
std::ostream& operator<<(std::ostream& stream, QwLogLevel level)
{
	switch (level) {
		case QwLogLevel::kError:   stream << "ERRO"; break;
		case QwLogLevel::kWarning: stream << "WARN"; break;
		case QwLogLevel::kVerbose: stream << "VERB"; break;
		case QwLogLevel::kDebug:   stream << "DEBG"; break;
		default:                   stream << "INFO"; break;
	}
	return stream;
}

// std:time_t is a typedef so ADL fails
struct Timestamp
{
	std::time_t time;
	explicit Timestamp(std::time_t t) : time(t) {}
	std::time_t* operator&() { return std::addressof(time); }
};
Timestamp GetTime()
{
	using namespace std::chrono;
	return Timestamp{system_clock::to_time_t(system_clock::now())};
}

std::ostream& operator<<(std::ostream& stream, Timestamp time)
{
	stream << std::put_time(std::localtime(&time), "%T");
	return stream;
}

QwLogProxy::QwLogProxy(VQwLogger& logger, QwLogLevel level)
: fLevel(level)
, fLogger(logger)
, fBuffer{}
{ }

QwLogProxy::QwLogProxy(QwLogProxy&& other) noexcept
: fLevel(other.fLevel)
, fLogger(other.fLogger)
, fBuffer(std::move(other.fBuffer))
{ }

void QwLogProxy::AddColor()
{
	fBuffer << GetLevelColor(fLevel);
}

void QwLogProxy::AddHeader()
{
	fBuffer << '[' << fLevel << " | " << GetTime() << "]: ";
}

void QwLogProxy::FlushBuffer()
{
	if(fBuffer.tellp() <= 0 ) return;

#if __cplusplus >= 202002L
	fLogger.Write(fLevel, fBuffer.view());
#else
	// Pre-c++20 incurs a copy
	fLogger.Write(fLevel, fBuffer.str());
#endif

	// Using an lvalue preserves the internal capacity
	static const std::string kEmptyStr;
	fBuffer.str(kEmptyStr);
	fBuffer.clear();
}

#if (__GNUC__ >= 3)
QwLogProxy& QwLogProxy::operator<<(std::ios_base& (*manip)(std::ios_base&))
{
	// Does not handle std::endl or std::flush properly
	fBuffer << manip;
    return *this;
}
#endif

QwLogProxy& QwLogProxy::operator<<(std::ostream& (*manip)(std::ostream&))
{
	using io_manip = std::ostream&(*)(std::ostream&);
	if( manip == static_cast<io_manip>(std::endl) ) {
		fBuffer << '\n';
		FlushBuffer();
	} else if( manip == static_cast<io_manip>(std::flush) ) {
		FlushBuffer();
	} else {
		fBuffer << manip;
	}
    return *this;
}

QwLogProxy::~QwLogProxy()
{
	FlushBuffer();
}

gQwLogger::gQwLogger()
: fScreenLogger(&std::cout, QwLogLevel::kMessage)
, fFileLogger(nullptr, QwLogLevel::kMessage)
, fPrintFunctionSignature{false}
, fUseColor{false}
{ }

gQwLogger& gQwLogger::Instance()
{
	static gQwLogger instance;
	return instance;
}

QwLogProxy gQwLogger::Log(QwLogLevel level, std::string_view msg)
{
	QwLogProxy llog = QwLogProxy{*this, level};

  	// Override log level of this sink when in a debugged function
  	if( IsDebugFunction(msg) ) { level = QwLogLevel::kAlways; }
	if( PrintWithColor() ) { llog.AddColor(); }
	if(level != QwLogLevel::kPlain) { llog.AddHeader(); }
	if( PrintFuncSignature() ) { llog << msg << " -> "; }

	return llog;
}

void gQwLogger::Write(QwLogLevel level, std::string_view log)
{
	fScreenLogger.Write(level, log);
	fFileLogger.Write(level, log);
}

void gQwLogger::InitLogFile(std::string const& name, const std::ios_base::openmode mode)
{
	std::ios_base::openmode flags = std::ios::out | mode;
	fFileLogger.SetStream(std::make_unique<std::ofstream>(name, flags));
}

/*!
 *  Determine whether the function name matches a specified list of regular expressions
 *  See https://lists.isocpp.org/std-proposals/att-0008/Dxxxx_string_view_support_for_regex.pdf
 *  for using std::string_view with std::regex
 */
bool gQwLogger::IsDebugFunction(std::string_view func_sig)
{
	// Using a temporary bool, we avoid acquiring the unique lock twice
	// but risk of running this loop N times for N threads (unlikely)
	bool is_debug_func = false;
	auto opt = fIsDebugFunction.Get(func_sig);
	if( !opt.has_value() ) {
		for (size_t i = 0; i < fDebugFunctionRegexString.size(); i++) {
			std::regex regex(fDebugFunctionRegexString.at(i));
			std::match_results<std::string_view::const_iterator> match_res;
			if (std::regex_match(func_sig.begin(), func_sig.end(), match_res, regex)) {
			    // When we find a match, break
				is_debug_func = true;
				break;
			}
		}
		// cache it for future lookups
		fIsDebugFunction.InsertOrAssign(func_sig, is_debug_func);
	} else {
		is_debug_func = *opt;
	}
	return is_debug_func;
}

bool gQwLogger::PrintFuncSignature() const { return fPrintFunctionSignature; }
bool gQwLogger::PrintWithColor()     const { return fUseColor; }


void gQwLogger::SetScreenColor(bool flag)
{
	fUseColor = flag;
}

void gQwLogger::SetScreenThreshold(int thr)
{
	fScreenLogger.SetLogLevel(thr);
}

void gQwLogger::SetFileThreshold(int thr)
{
	fFileLogger.SetLogLevel(thr);
}

QwLogLevel gQwLogger::GetLogLevel() const {
	return std::max(fScreenLogger.GetLogLevel(), fFileLogger.GetLogLevel());
};

std::ostream& QwLog::endl(std::ostream& stream) { return std::endl(stream); }




/**
 * Defines configuration options for logging class using QwOptions
 * functionality.
 *
 * Note: this uses a pointer as opposed to a reference, because as indicated
 * above the QwLog class cannot depend on the QwOptions class.  When using a
 * pointer we only need a forward declaration and we do not need to include
 * the header file QwOptions.h.
 *
 * @param options Options object
 */
void gQwLogger::DefineOptions(QwOptions* options)
{
	// Define the logging options
	options->AddOptions("Logging options")("QwLog.color",
			po::value<bool>()->default_value(true),
			"colored screen output");
	options->AddOptions("Logging options")("QwLog.logfile",
			po::value<std::string>(),
			"log file");
	options->AddOptions("Logging options")("QwLog.loglevel-file",
			po::value<int>()->default_value(static_cast<int>(QwLogLevel::kMessage)),
			"log level for file output");
	options->AddOptions("Logging options")("QwLog.loglevel-screen",
			po::value<int>()->default_value(static_cast<int>(QwLogLevel::kMessage)),
			"log level for screen output");
	options->AddOptions("Logging options")("QwLog.print-signature",
			po::value<bool>()->default_bool_value(false),
			"print signature on error or warning");
	options->AddOptions("Logging options")("QwLog.debug-function",
			po::value< std::vector<std::string> >()->multitoken(),
			"print debugging output of function with signatures satisfying the specified regex");
}

/**
 * Process configuration options for logging class using QwOptions
 * functionality.
 *
 * Note: this uses a pointer as opposed to a reference, because as indicated
 * above the QwLog class cannot depend on the QwOptions class.  When using a
 * pointer we only need a forward declaration and we do not need to include
 * the header file QwOptions.h.
 *
 * @param options Options object
 */
void gQwLogger::ProcessOptions(QwOptions* options)
{
  // Initialize log file
  if (options->HasValue("QwLog.logfile"))
    InitLogFile(options->GetValue<std::string>("QwLog.logfile"));

  // Set the logging thresholds
  SetFileThreshold(options->GetValue<int>("QwLog.loglevel-file"));
  SetScreenThreshold(options->GetValue<int>("QwLog.loglevel-screen"));

  // Set color flag
  SetScreenColor(options->GetValue<bool>("QwLog.color"));

  // Set the flags for function name and signature printing
  fPrintFunctionSignature = options->GetValue<bool>("QwLog.print-signature");

  // Set the list of regular expressions for functions to debug
  fDebugFunctionRegexString = options->GetValueVector<std::string>("QwLog.debug-function");
  if (fDebugFunctionRegexString.size() > 0)
    std::cout << "Debug regex list:" << std::endl;
  for (size_t i = 0; i < fDebugFunctionRegexString.size(); i++) {
    std::cout << fDebugFunctionRegexString.back() << std::endl;
  }
}
