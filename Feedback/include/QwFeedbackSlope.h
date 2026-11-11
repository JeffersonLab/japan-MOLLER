#pragma once
#include <array>
#include "VQwEnumArray.h"
#include "QwIHWP.h"
#include "QwPublisher.h"
#include "QwFeedbackLogger.h"

/**
 * @class QwIHWPSlope
 * @brief Feedback Slope based off of the IHWP state
 * @details
 *        Enum-indexed array contaning the feedback slopes
 *        that vary based off of the IHWP state. 
 *
 *        The IHWP state is updated atomically by the IWHP_IOC container
 *        (see QwIHWP.h), and the corresponding slope can be accessed via
 *        a calls to VQwFeedbackSlopes::Get();
 */
class QwIHWPSlope : public VQwFeedbackSlopes
{
	static_assert( static_cast<int>(IHWP::kIN) == 0
			&& static_cast<int>(IHWP::kOUT) == 1,
			"Expected: enum IHWP is used for indexing!\n");
	static constexpr std::size_t ARR_SIZE =  IHWP_IOC::fNumPossibleStates;
	std::array<double, ARR_SIZE> fSlopes;
	IHWP_IOC fIHWP;
	mutable bool fIsValid{false};

	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
	void    check_isvalid() const;
public:
	QwIHWPSlope(QwFeedbackLogger& logger);
	std::unique_ptr<VQwFeedbackSlopes> Clone() const override;
	bool IsValid() const;
};

/**
 * @class QwPOSXYSlope
 * @brief Feedback Slope based off of the four linear combinations of XU, XV, YU, YV
 * @details
 *        Enum-indexed array contaning the feedback slopes
 *        that vary based off of the four lin. combinations
 *
 *        The corresponding slope can be accessed via
 *        a calls to VQwFeedbackSlopes::Get(POS_TYPE::);
 *        e.g.
 *          slope->Get(QwPOSXYSlope::POS_TYPE::kYV)
 *	 		slope->Get(QwPOSXYSlope::POS_TYPE::kXV)
 *	 		slope->Get(QwPOSXYSlope::POS_TYPE::kYU)
 *	 		slope->Get(QwPOSXYSlope::POS_TYPE::kXU)
 *
 *
 *        On the first call to VQwFeedbackSlopes::Get, the linear combinations
 *        are calculated and cached
 */
class QwPOSXYSlope : public VQwFeedbackSlopes, public Publisher<QwFeedbackLogPayload>
{ 
public:
	enum class POS_TYPE
	{
		kYV = 0, kYU = 1,
		kXV = 2, kXU = 3
	};
	static_assert( static_cast<int>(POS_TYPE::kYV) == 0
			&& static_cast<int>(POS_TYPE::kXU) == 3,
			"Expected: enum POS_TYPE is used for indexing!\n");
	QwPOSXYSlope(QwFeedbackLogger& logger);
	std::unique_ptr<VQwFeedbackSlopes> Clone() const override;
	void    AttachLogger(QwFeedbackLogger& logger);
	bool    IsValid() const;
private:
	void    SetImpl(int index, const double val) override;
	double  GetImpl(int index) const override;
	void    cache() const;
	void    check_isvalid() const;

	static constexpr std::size_t ARR_SIZE = 4;
	std::array<double, ARR_SIZE> fSlopes_raw{};
	mutable std::array<double, ARR_SIZE> fSlopes_cached{};
	mutable bool fIsCached{false};
	mutable bool fIsValid{false};
};
