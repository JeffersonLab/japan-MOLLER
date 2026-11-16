#pragma once
#include <ostream>
#include <string>
#include <string_view>
#include "QwLimits.h"

/**
 * @class QwFeedbackConfig
 * @brief Contains all the Feedback configuration options
 * @par aposu_feedback.map
 * type=target name=x_targ            descr=DIFFX
 * type=ioc    name=RTPXPORT1Q1calc.A descr=PC_HV1 lower=-1.0 upper=36000
 * type=ioc    name=RTPXPORT1Q4calc.A descr=PC_HV2 lower=-1.0 upper=36000
 * type=ioc    name=RTPXPORT1Q2calc.A descr=PC_HV5 lower=-1.0 upper=36000
 * type=ioc    name=RTPXPORT1Q3calc.A descr=PC_HV6 lower=-1.0 upper=36000
 */
class QwFeedbackConfig
{
public:
	/**
	 * @enum TYPE
	 * @brief Configuration type
	 */
	enum class TYPE	{ kTARGET, kIOC, kUNKNOWN };
private:
	TYPE type{TYPE::kUNKNOWN};
	std::string name;
	std::string descr;
	QwSetpointLimits limits{};

	std::string_view parse_pair_impl(std::string_view token, std::string_view target);
public:
	// Config Parsing
	bool parse_pair(std::string_view token);
	[[nodiscard]] bool isValid() const;

	// Accesors
	TYPE getType() const;
	std::string const& getName() const noexcept;
	std::string const& getDescr() const noexcept;
	QwSetpointLimits const& getLimits() const noexcept;

	// We all need friends
	friend std::ostream& operator<<(std::ostream& out, QwFeedbackConfig const& config);
};


