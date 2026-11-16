#pragma once
#include <string>
#include <string_view>
#include "QwFeedbackSetpoints.h"
#include "QwFeedbackLogger.h"
#include "QwFeedbackStrategy.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackCorrectionPayload.h"
#include "QwLimits.h"


/**
 * @class VQwFeedbackImpl
 * @brief Feedback Impl Interface
 */
class VQwFeedbackImpl
{
public:
	virtual void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) = 0;
	virtual std::unique_ptr<VQwFeedbackImpl> Clone() const = 0;
	virtual bool AddSetpoint(std::string const& setpoints, QwSetpointLimits const&) = 0;
	virtual std::string_view GetName() const = 0;
};

/**
 * @class QwFeedbackImpl
 * @brief Feedback Impl using the strategy (policy) pattern
 * @tparam FeedbackStrategy Feedback strategy
 */
template<typename FeedbackStrategy>
class QwFeedbackImpl : public VQwFeedbackImpl
{
	FeedbackStrategy fFbStrategy;
	QwFeedbackLogger& fLogger;
public:
	QwFeedbackImpl(QwFeedbackLogger& logger);
	void ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope) override;
	std::unique_ptr<VQwFeedbackImpl> Clone() const override;
	bool AddSetpoint(std::string const& setpoint, QwSetpointLimits const&) override;
	std::string_view GetName() const override;
};

template<typename FeedbackStrategy>
QwFeedbackImpl<FeedbackStrategy>::QwFeedbackImpl(QwFeedbackLogger& logger)
: fFbStrategy{}
, fLogger(logger)
{ }

template<typename FeedbackStrategy>
bool QwFeedbackImpl<FeedbackStrategy>::AddSetpoint(std::string const& setpoint, QwSetpointLimits const& limits)
{
	return fFbStrategy.SetSetpoint(setpoint.c_str(), limits, fLogger);
}

template<typename FeedbackStrategy>
void QwFeedbackImpl<FeedbackStrategy>::ApplyCorrectionImpl(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope)
{
	fFbStrategy(payload, slope);
}


template<typename FeedbackStrategy>
std::unique_ptr<VQwFeedbackImpl> QwFeedbackImpl<FeedbackStrategy>::Clone() const
{
	return std::make_unique<QwFeedbackImpl>( *this );
}

template<typename FeedbackStrategy>
std::string_view QwFeedbackImpl<FeedbackStrategy>::GetName() const { return fFbStrategy.GetName(); }
