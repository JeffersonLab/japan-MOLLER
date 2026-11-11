#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "VQwDataHandler.h" // Access EQwHandleType
#include "QwFeedbackLogger.h"
#include "QwFeedbackSlope.h"
#include "QwFeedbackSetpoints.h"
#include "QwFeedbackCorrectionPayload.h"

class VQwFeedbackImpl;
class QwFeedbackConfig;

/**
 * @class Device
 * @brief Encapsulates the information for a specific VQwhardwareChannel device 
 */
struct Device
{
	Device(std::string name, std::string_view descr)
	: fName(std::move(name))
	, fFbType(GetFbCorrType(descr))
	, fQwType{VQwDataHandler::kHandleTypeUnknown}
	{
		if(fFbType == FbCorrType::ASYM) {
			fQwType = VQwDataHandler::kHandleTypeAsym;
		} else if(fFbType == FbCorrType::DIFFX || fFbType == FbCorrType::DIFFY) {
			fQwType = VQwDataHandler::kHandleTypeDiff;
		}
	}
	std::string   fName;
	FbCorrType    fFbType;
	VQwDataHandler::EQwHandleType fQwType;
};

/**
 * @class QwFeedback
 * @brief QwFeedbackHandler and QwFeedbackImpl bridge. Contains intermediate logic
 *        such as list of VQwHardChannel devices, feedback slopes, and configuration.
 */
class QwFeedback
{
	/*@var fPimpl pointer to feedback impl */
	std::unique_ptr<VQwFeedbackImpl>   fPimpl;

	/*@var fPimpl pointer to slope impl */
	std::unique_ptr<VQwFeedbackSlopes> fSlope;

	QwFeedbackLogger fLogFile;
	std::vector<Device> fDevices;
public:
	/*@enum Supported Feedback Types */
	enum class TYPE {
		PITA,
		APOSU,
		APOSV,
		IA,
		POSXY,
		UNKNOWN
	};
private:
	/**
	 *@brief Instantiations the correct slope and feedback impl based off of the feedback type
	 *@param [in] type Feedback type
	 *@details No Op if feedback impl is already instantiated
	 */
	void ConfigureFeedbackType(TYPE type);
public:
	QwFeedback();
	QwFeedback(QwFeedback const& other);

	/**
	 *@brief Obtains Feedback type from string like input
	 *@param [in] sv string like input
	 */
	TYPE GetTypeOfFeedback(std::string_view sv) const;

	/**
	 *@brief Determines the feedback type and instantiates it alongside the proper slope impl
	 *@param [in] sv string like input
	 @retval Feedback type selected
	 */
	TYPE ConfigureFeedbackType(std::string_view);

	/**
	 *@brief Configures the logfile
	 @retval success
	 */
	bool ConfigureLogger(std::string const&, std::ios_base::openmode mode = std::ios::app);

	/**
	 *@brief Configure the feedback system
	 */
	void ConfigureFeedback(QwFeedbackConfig const& config);

	/**
	 *@brief Applies the correction. No op if fSlope or fPimpl is null
	 */
	void ApplyCorrection(QwFeedbackCorrectionPayload const& payload);

	/**
	 *@brief Get list of VQwHardware devices
	 */
	std::vector<Device> const& RequestTargetDevices() const;

	/**
	 *@brief Get the name of the feedback system
	 */
	std::string_view RequestFeedbackName() const;

	/**
	 *@brief Set the value of the slope for a specific index.
	 *       No op of fSlopes is null
	 *@tparam IndexType Enumerated Index
	 */
	template<typename IndexType>
	void SetSlope(IndexType Index, double val);
};

template<typename IndexType>
void QwFeedback::SetSlope(IndexType index, double val)
{
	if(fSlope) fSlope->Set<IndexType>(index, val);
}

