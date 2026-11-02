/*!
 * \file   QwLog.h
 * \brief  A logfile class, based on an identical class in the Hermes analyzer
 *
 * \author Wouter Deconinck
 * \date   2009-11-25
 */

#pragma once
#include <iostream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <sstream>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include "QwColor.h"
#include "QwThreadSafeMap.h"

/*!
 * \note Because QwOptions depends on QwLog, and QwLog depends also on QwOptions,
 * we cannot include QwOptions in the QwLog header file here.  QwLog is treated
 * as more basic than QwOptions.
 */

// Forward declarations
class QwOptions;


/*! \def QwOut
 *  \brief Predefined log drain for explicit output
 */
#define QwOut      gQwLogger::Instance().Log(QwLogLevel::kPlain,__PRETTY_FUNCTION__)

/*! \def QwError
 *  \brief Predefined log drain for errors
 */
#define QwError    gQwLogger::Instance().Log(QwLogLevel::kError,__PRETTY_FUNCTION__)

/*! \def QwWarning
 *  \brief Predefined log drain for warnings
 */
#define QwWarning  gQwLogger::Instance().Log(QwLogLevel::kWarning,__PRETTY_FUNCTION__)

/*! \def QwMessage
 *  \brief Predefined log drain for regular messages
 */
#define QwMessage  gQwLogger::Instance().Log(QwLogLevel::kMessage,__PRETTY_FUNCTION__)

/*! \def QwVerbose
 *  \brief Predefined log drain for verbose messages
 */
#define QwVerbose  if (auto& glog = gQwLogger::Instance();\
						glog.GetLogLevel() >= QwLogLevel::kVerbose) glog.Log(QwLogLevel::kVerbose,__PRETTY_FUNCTION__)

/*! \def QwDebug
 *  \brief Predefined log drain for debugging output
 */
#define QwDebug    if (auto& glog = gQwLogger::Instance();\
						glog.GetLogLevel() >= QwLogLevel::kDebug) glog.Log(QwLogLevel::kDebug,__PRETTY_FUNCTION__)

enum class QwLogLevel : int {
	kPlain     = -2, /*!< Explicit output  w/o TS */
	kAlways    = -1, /*!< Explicit output  */
	kError     =  0, /*!< Error loglevel   */
	kWarning   =  1, /*!< Warning loglevel */
	kMessage   =  2, /*!< Message loglevel */
	kVerbose   =  3, /*!< Verbose loglevel */
	kDebug     =  4  /*!< Debug loglevel   */
};
QwLogLevel ConvertToEnum(int thr);
QwColor GetLevelColor(QwLogLevel level);
std::ostream& operator<<(std::ostream& stream, QwLogLevel level);

// VQwLogger depends on QwLogProxy
// and QwLogProxy Depends on VQwLogger
class QwLogProxy;
class VQwLogger
{
public:
	virtual void Write(QwLogLevel level, std::string_view log) = 0;
	virtual QwLogProxy Log(QwLogLevel level, std::string_view msg = std::string_view{}) = 0;
};

class QwLogProxy
{
	QwLogLevel fLevel;
	VQwLogger& fLogger;
	std::ostringstream fBuffer;
private:
	QwLogProxy& operator=(QwLogProxy && other) = delete;
	QwLogProxy(QwLogProxy const& other) = delete;
	QwLogProxy& operator=(QwLogProxy const& other) = delete;
	void FlushBuffer();
public:
	QwLogProxy(VQwLogger& logger, QwLogLevel level);
	QwLogProxy(QwLogProxy&& other) noexcept;
	~QwLogProxy();
	template<typename T>
	QwLogProxy& operator<<(T const& val);
#if (__GNUC__ >= 3)
	QwLogProxy& operator<<(std::ios_base& (*manip)(std::ios_base&));
#endif
	QwLogProxy& operator<<(std::ostream& (*manip)(std::ostream&));
public:
	// Class Style Modifiers
	void AddColor();
	void AddHeader();
};

// Idea, the PtrLikeType
// handles the memory management
// std::cout -> ostream*
// std::ofstream -> unique_ptr
template<typename PtrLikeType>
class QwLogger : public VQwLogger
{
	mutable std::mutex fStreamMutex;
	PtrLikeType fStream;
	// Assuming std::hardware_destructive_interference_size = 64
	alignas(64) std::atomic<QwLogLevel> fThreshold;

	QwLogger(QwLogger const&) = delete;
	QwLogger& operator=(QwLogger const&) = delete;
public:

	/*! \brief Default Ctor: Sets the QwLogLevel and stream sink to null
	 *  \param QwLogLevel threshold: Threshold level
	 */
	explicit QwLogger(QwLogLevel threshold = QwLogLevel::kMessage);
	/*! \brief Ctor: Sets the QwLogLevel and stream sink
	 *  \param U&& Stream: Forwarding Reference to a PtrLike sink (raw, smart, etc)
	 *  \param QwLogLevel threshold: Threshold level
	 */
	template<typename U>
	QwLogger(U&& stream, QwLogLevel threshold = QwLogLevel::kMessage);
	QwLogger(QwLogger&& other) noexcept;
	QwLogger& operator=(QwLogger && other) noexcept;

	/*! \brief Create a LogProxy to handle logging
	 */
	QwLogProxy Log(QwLogLevel level, std::string_view msg = std::string_view{}) override;

    /*! \brief Write to stream
     */
	void Write(QwLogLevel level, std::string_view log) override;

    /*! \brief Set the log threshold
     */
    void SetLogLevel(int thr) noexcept;

    /*! \brief Get log threshold
     */
    QwLogLevel GetLogLevel() const noexcept;

    /*! \brief Resets the stream sink
     */
	void SetStream(PtrLikeType stream) noexcept;

    /*! \brief Bool operator
     */
	explicit operator bool() const;
};


class gQwLogger : public VQwLogger
{

    //! File thresholds and stream
	QwLogger<std::ostream*> fScreenLogger;
    //! File thresholds and stream
	QwLogger<std::unique_ptr<std::ostream>> fFileLogger;


    //! List of regular expressions for functions that will have increased log level
    QwThreadSafeMap<std::string,bool> fIsDebugFunction;
    std::vector<std::string> fDebugFunctionRegexString;

    //! Flag to print function signature on warning or error
    bool fPrintFunctionSignature;
    //! Flag to disable color
    bool fUseColor;

	gQwLogger();
public:
	/*! \brief Access the singleton
	 */
	static gQwLogger& Instance();
	/*! \brief Create a LogProxy to handle logging
	 */
	QwLogProxy Log(QwLogLevel level, std::string_view msg = std::string_view{}) override;
    /*! \brief Write to stream
     */
	void Write(QwLogLevel level, std::string_view log) override;


	/* Confuguration Accessors */
    /// \brief Define available class options for QwOptions
    static void DefineOptions(QwOptions* options);

    /// \brief Process class options for QwOptions
    // Note: this uses pointers as opposed to references, because as indicated
    // above the QwLog class cannot depend on the QwOptions class.  When using a
    // pointer we only need a forward declaration and we do not need to include
    // the header file QwOptions.h.
    void ProcessOptions(QwOptions* options);

	/* Confuguration Accessors */
	/*! Initialize the log file with name 'name'
	*/
	void InitLogFile(std::string const& name, const std::ios_base::openmode mode = std::ios::app);
	bool IsDebugFunction(std::string_view func_sig);
	bool PrintFuncSignature() const;
    /*! \brief Set the screen color mode
     */
    void SetScreenColor(bool flag);
	bool PrintWithColor() const;

    /*! \brief Set the screen log level
     */
    void SetScreenThreshold(int thr);
    /*! \brief Set the file log level
     */
    void SetFileThreshold(int thr);

    /*! \brief Get highest log level
     */
    QwLogLevel GetLogLevel() const;

};

struct QwLog
{
	/*! QwLog::endl
	 *  \brief Backward compat.
	 */
	static std::ostream& endl(std::ostream& stream);
};


template<typename T>
QwLogProxy& QwLogProxy::operator<<(T const& val)
{
	fBuffer << val;
	return *this;
}

template<typename PtrLikeType>
QwLogger<PtrLikeType>::QwLogger(QwLogLevel threshold)
: fStream{nullptr}
, fThreshold(threshold)
{ }

template<typename PtrLikeType> template<typename U>
QwLogger<PtrLikeType>::QwLogger(U&& stream, QwLogLevel threshold)
: fStream{std::forward<U>(stream)}
, fThreshold(threshold)
{ }

template<typename PtrLikeType>
void QwLogger<PtrLikeType>::Write(QwLogLevel level, std::string_view log)
{
	// Only acquire the lock if necessary
	if( level <= fThreshold.load(std::memory_order_acquire) ) {
		std::lock_guard lk(fStreamMutex);
		if(fStream) {
			*fStream << log;
		}
	}
}

template<typename PtrLikeType>
void QwLogger<PtrLikeType>::SetLogLevel(int thr) noexcept
{
	fThreshold.store(ConvertToEnum(thr), std::memory_order_release);
}

template<typename PtrLikeType>
QwLogLevel QwLogger<PtrLikeType>::GetLogLevel() const noexcept
{
	return fThreshold.load(std::memory_order_acquire);
}
template<typename PtrLikeType>
void QwLogger<PtrLikeType>::SetStream(PtrLikeType stream) noexcept
{
	std::lock_guard lk(fStreamMutex);
	fStream = std::move(stream);
}
template<typename PtrLikeType>
QwLogger<PtrLikeType>::QwLogger(QwLogger&& other) noexcept
{
	std::scoped_lock lk(fStreamMutex, other.fStreamMutex);
	fStream = std::move(other.fStream);
	fThreshold.store(other.fThreshold.load(std::memory_order_relaxed),
			           std::memory_order_relaxed);
}
template<typename PtrLikeType>
QwLogger<PtrLikeType>& QwLogger<PtrLikeType>::operator=(QwLogger && other) noexcept
{
	if(this == &other) return *this;
	std::scoped_lock lk(fStreamMutex, other.fStreamMutex);
	fStream = std::move(other.fStream);
	fThreshold.store(other.fThreshold.load(std::memory_order_relaxed),
						std::memory_order_relaxed);
	return *this;
}
template<typename PtrLikeType>
QwLogger<PtrLikeType>::operator bool() const
{
	return static_cast<bool>(fStream) && static_cast<bool>(*fStream);
}

template<typename PtrLikeType>
QwLogProxy QwLogger<PtrLikeType>::Log(QwLogLevel level, std::string_view msg)
{
	QwLogProxy log = QwLogProxy{*this, level};
	log.AddHeader();
	log << msg;
	return log;
}

