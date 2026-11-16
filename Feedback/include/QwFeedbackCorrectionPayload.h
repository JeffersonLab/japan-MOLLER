#pragma once
#include <array>
#include <cassert>
#include <memory>
#include <string_view>
#include "VQwEnumArray.h"

/**
 * @enum FbCorrType
 * @brief Feedback Correction type
 */
enum class FbCorrType
{
	ASYM,
	DIFFX,
	DIFFY
};

// Useful standalone util functions
std::string_view stringify(FbCorrType type);
FbCorrType GetFbCorrType(std::string_view);

/**
 * @class QwFeedbackCorrectionPayload
 * @brief Payload used for feedback corrections.
 * @details
 *        Implemented as a wrapper around an array to allow
 *        for the usage 'corr = payload.Get[DIFFX] / fSlope->Get()'
 *
 *        This provides a readable mapping between device types and
 *        feedback corrections. Yes this is not an ideal implementation,
 *        but I needed a clean way to map specifics correction
 *        (Y corrections versus X corrections versus ASYM corrections)
 *        to different Correction strategies (aposU, aposV, PITA, POSXY)
 *        in a type safe way (provided by asserts and use of enum indexing).
 *       
 */
class QwFeedbackCorrectionPayload : public VQwFeedbackValues
{
	static_assert( static_cast<int>(FbCorrType::ASYM) == 0
			&& static_cast<int>(FbCorrType::DIFFY) == 2,
			"Expected: enum POS_TYPE is used for indexing!\n");

	static constexpr std::size_t ARR_SIZE = 3;
	std::array<double, ARR_SIZE> fValues{};

	std::unique_ptr<VQwFeedbackValues> Clone() const override;
public:
	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
};
