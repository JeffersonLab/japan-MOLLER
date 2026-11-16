#pragma once
#include <future>
#include <string>
#include "QwFeedbackLogger.h"
#include "QwAtomicEpicTypes.h"
#include "QwEpicChannel.h"
#include "QwIHWP.h"
#include "QwLimits.h"

/**
 * @class QwFeedbackSetpoint
 * @brief Handles the logic of applying corrections for an individual IOC
 * @details
 * 		  Instead of requesting (and waiting) for up-to-date EPICS values every time
 * 		  we need them, we can let the EPICS lib handle the updates for us. This is
 * 		  achieved using the Observer pattern and the EPICS lib callback functionality.
 * 		  When a IOC value changes, the EPICS lib updates our held value atomically.
 * 		  When we write, we use the standard EpicChannel::PutAsync. The caller can decide
 * 		  to block on the std::future<void> or not. 
 *
 * 		  Logging is handled in a similar manner. A logger (observer) is attached to the
 * 		  setpoint (publisher). Everytime we request or receive a change in the setpoint value,
 * 		  we notify the logger of the change.
 */
class QwFeedbackSetpoint : public EPICSObserver<double>, public Publisher<QwFeedbackLogPayload>
{
private:
    /*@var fChannel the EPICS IOC */
	EpicChannel* fChannel;

    /*@var fCorr
	 *@brief The last applied correction. Updated on calls to ApplyCorrection()
	 */
	double fCorr;

    /*@var fPrev
	 *@brief The previous setpoint value. Updated on calls to Update()
	 */
	double fPrev;

    /*@var fLimits
	 *@brief Setpoint limits. Corrected setpoints outside this range are not sent
	 */
	QwSetpointLimits fLimits;

    /*@var fCurr
	 *@brief Current setpoint value. Updated in calls to Update()
     *       Read in calls to ApplyCorrection().
	 *       The thread that writes to the variable is likely not
	 *       the thread that reads it
	 */
	AtomicEpicsType<double> fCurr;

    /*@var fName
	 *@brief Cached fChannel name
	 */
	std::string fName;

	bool IsWithinLimits(double const input) const noexcept;
public:
	/**
	 *@brief Attaches a EpicChannel to the setpoint
	 *@details If a channel is already attached, it
	 *         de-attaches and attaches the new channel
	 */
	void Attach(const char* pv_name);
	void SetLimits(QwSetpointLimits const& limits);

	/**
	 *@brief Inherited from EPICSObserver<>
	 */
	void Update(double const& data) override;

	/**
	 *@brief Applies the correction
	 *@param [in] corr Correction value to apply
	 *@tparam BinaryOp Binary transformation between correction value and current value
	 */
	template<typename BinaryOp>
	std::future<void> ApplyCorrection(double corr);
public:
	QwFeedbackSetpoint();
	QwFeedbackSetpoint(EpicChannel* channel);
	~QwFeedbackSetpoint();
	QwFeedbackSetpoint(QwFeedbackSetpoint const& other);
	QwFeedbackSetpoint(QwFeedbackSetpoint&& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint const& other) = delete;
	QwFeedbackSetpoint& operator=(QwFeedbackSetpoint&& other) = delete;
};

template<typename BinaryOp>
std::future<void> QwFeedbackSetpoint::ApplyCorrection(double corr)
{
	if(!fChannel) {
		std::string log_payload = "Attempting to send request to "
			+ fName
			+ ", but is null... something bad has happened!\n";
		Notify(QwFeedbackLogPayload{QwLogLevel::kError, std::move(log_payload)});
		return std::future<void>{};
	}

	double current = fCurr.Load(std::memory_order_acquire);
	fCorr = BinaryOp{}(current, corr);
	if(IsWithinLimits(fCorr) == false) {
		std::string log_payload = "Request: " + fName
			+ ": fPrev = "        + std::to_string(fPrev)
			+ ", fCurrent = "     + std::to_string(current)
			+ ", Correction = "   + std::to_string(fCorr)
			+ " is outside of limits [" + std::to_string(fLimits.lower_limit)
			+ ", " + std::to_string(fLimits.upper_limit) + "]\n";
		Notify(QwFeedbackLogPayload{QwLogLevel::kError, std::move(log_payload)});
		return std::future<void>{};
	}
	auto fut = fChannel->PutAsync(fCorr);
	std::string log_payload = "Request: " + fName
			+ ": fPrev = "        + std::to_string(fPrev)
			+ ", fCurrent = "     + std::to_string(current)
			+ ", Correction = "   + std::to_string(fCorr) + '\n';
	QwMessage << fName << " | " << log_payload;
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)});
	return fut;
}

