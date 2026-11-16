#pragma once
#include <string_view>
#include <array>
#include "QwLog.h"
#include "QwFeedbackSetpoints.h"
#include "QwFeedbackCorrectionPayload.h"
#include "QwLimits.h"

/**
 * @class QwPitaFBStrategy
 * @brief PITA Feedback Strategy
 */
struct QwPitaFBStrategy
{
	static constexpr std::string_view fName{"PITA Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{8};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints);
};

/**
 * @class QwAposUFBStrategy
 * @brief AposU Feedback Strategy
 */
struct QwAposUFBStrategy
{
	static constexpr std::string_view fName{"AposU Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints);
};

/**
 * @class QwAposVFBStrategy
 * @brief AposV Feedback Strategy
 */
struct QwAposVFBStrategy
{
	static constexpr std::string_view fName{"AposV Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints);
};


/**
 * @class QwIAFBStrategy
 * @brief IA Feedback Strategy
 */
struct QwIAFBStrategy
{
	static constexpr std::string_view fName{"IA Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{4};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints);
};

/**
 * @class QwPosXYFBStrategy
 * @brief POSXY Feedback Strategy
 */
struct QwPosXYFBStrategy
{
	static constexpr std::string_view fName{"PosXY Feedback"};
	static constexpr std::size_t fNumSetpointsExpected{8};
	using SetPointArray = std::array<QwFeedbackSetpoint, fNumSetpointsExpected>;
	static void Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints);
};

/**
 * @class QwFBStrategy
 * @brief Generic Feedback Strategy. Isolates common functinality
 *        and specifics are injected using the sub-strategies (policies)
 */
template <typename Strategy>
class QwFBStrategy
{
	static constexpr std::string_view fName            = Strategy::fName;
	static constexpr std::size_t fNumSetpointsExpected = Strategy::fNumSetpointsExpected;
	std::size_t fNumSetpointsSet{0};
	std::array<QwFeedbackSetpoint, fNumSetpointsExpected> fSetpoints;
public:
	/**
	 *@brief Add an IOC (setpoint)
	 *@param [in] pv_name Process variable
	 *@param [in] limits Correction limits
	 *@param [in] logger Logger to attach to setpoint
	 *@details Requests the IOC from QwEpicHandler.
	 *@retval  TRUE : Setpoint added; FALSE : Setpoint not added
	 */
	bool SetSetpoint(const char* pv_name, QwSetpointLimits const& limits, QwFeedbackLogger& logger);

	/**
	 *@brief Sends the corrections and slopes to each setpoint for processing
	 *@param [in] payload Measured values to correct
	 *@param [in] slope value to setpoint unit conversion
	 */
	void operator()(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope);

	/**
	 *@brief Get the name of the Feedback Strategy
	 */
	constexpr std::string_view GetName() const;
};

template <typename Strategy>
bool QwFBStrategy<Strategy>::SetSetpoint(const char* pv_name, QwSetpointLimits const& limits, QwFeedbackLogger& logger)
{
	if(fNumSetpointsSet >= fNumSetpointsExpected) {
		QwError << "Error: " << fName << " Setpoints already set!\n";
		return false;
	}
	QwMessage << "Adding Setpoint: " << pv_name << '\n';
	auto& setpoint = fSetpoints[fNumSetpointsSet++];
	setpoint.Attach(pv_name);
	setpoint.AddObserver(&logger);
	setpoint.SetLimits(limits);
	return true;
}

template <typename Strategy>
void QwFBStrategy<Strategy>::operator()(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope)
{
	if(fNumSetpointsSet == fNumSetpointsExpected) {
		Strategy::Apply(payload, slope, fSetpoints);
	} else {
		QwError << fName << " does not have the expected number of setpoints set!"
		        << "\n\tNo. Set: "      << fNumSetpointsSet 
		        << "\n\tNo. Expected: " << fNumSetpointsExpected
				<< "\nCorrection not applied!" << QwLog::endl;
	}
}

template <typename Strategy> constexpr
std::string_view QwFBStrategy<Strategy>::GetName() const
{
	return fName;
}


// Helpful Strategy Aliases
using QwPitaStrategy  = QwFBStrategy<QwPitaFBStrategy>;
using QwAposUStrategy = QwFBStrategy<QwAposUFBStrategy>;
using QwAposVStrategy = QwFBStrategy<QwAposVFBStrategy>;
using QwIAStrategy    = QwFBStrategy<QwIAFBStrategy>;
using QwPosXYStrategy = QwFBStrategy<QwPosXYFBStrategy>;
