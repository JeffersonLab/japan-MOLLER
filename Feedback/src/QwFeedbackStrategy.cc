#include "QwFeedbackStrategy.h"
#include "QwFeedbackSlope.h"

#define WAIT_FOR_TX_ACK

template <std::size_t N>
void wait_for_tx_ack(std::string_view name, std::array<std::future<void>, N>& ack) {
	for( auto& tx : ack ) {
		try{
			tx.get();
		} catch(std::future_error const& except) {
			QwError << name << " Request not sent! " << except.what() << QwLog::endl;
		} catch(std::runtime_error const& except) {
			QwError << name << " Request not sent! " << except.what() << QwLog::endl;
		}
	}
}

void QwPitaFBStrategy::Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	double const correction = payload.Get(FbCorrType::ASYM) / slope->Get();
	for(std::size_t index = 0; index < N; index++) {
		if(index < 4) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		}
	}
#ifdef WAIT_FOR_TX_ACK
	wait_for_tx_ack(fName, ack);
#endif
}


void QwAposUFBStrategy::Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	double const correction = payload.Get(FbCorrType::ASYM) / slope->Get();
	for(std::size_t index = 0; index < N; index++) {
		if(index % 2) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		}
	}

#ifdef WAIT_FOR_TX_ACK
	wait_for_tx_ack(fName, ack);
#endif
}


void QwAposVFBStrategy::Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	double const correction = payload.Get(FbCorrType::ASYM) / slope->Get();
	for(std::size_t index = 0; index < N; index++) {
		if(index % 2) {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
		} else {
			ack[index] = std::move( setpoints[index].ApplyCorrection<std::plus<double>>(correction) );
		}
	}

#ifdef WAIT_FOR_TX_ACK
	wait_for_tx_ack(fName, ack);
#endif
}


void QwIAFBStrategy::Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	double const correction = payload.Get(FbCorrType::ASYM) / slope->Get();
	for(std::size_t index = 0; index < N; index++) {
		ack[index] = std::move( setpoints[index].ApplyCorrection<std::minus<double>>(correction) );
	}

#ifdef WAIT_FOR_TX_ACK
	wait_for_tx_ack(fName, ack);
#endif
}


void QwPosXYFBStrategy::Apply(QwFeedbackCorrectionPayload const& payload, VQwFeedbackSlopes const* slope, SetPointArray& setpoints)
{
	using tx_ack = std::future<void>;
	constexpr auto N = fNumSetpointsExpected;
	std::array<tx_ack, N> ack;
	double const diff_x  = payload.Get(FbCorrType::DIFFX);
	double const diff_y  = payload.Get(FbCorrType::DIFFY);

	double const corr_vx = slope->Get(QwPOSXYSlope::POS_TYPE::kYV) * diff_x;
	double const corr_vy = slope->Get(QwPOSXYSlope::POS_TYPE::kXV) * diff_y;
	double const corr_ux = slope->Get(QwPOSXYSlope::POS_TYPE::kYU) * diff_x;
	double const corr_uy = slope->Get(QwPOSXYSlope::POS_TYPE::kXU) * diff_y;

	double const corr_v = corr_vx + corr_vy;
	double const corr_u = corr_ux + corr_uy;
	
	ack[0] = std::move( setpoints[0].ApplyCorrection<std::minus<double>>(corr_v) );
	ack[1] = std::move( setpoints[1].ApplyCorrection<std::plus<double>>(corr_v)  );
	ack[2] = std::move( setpoints[2].ApplyCorrection<std::minus<double>>(corr_u) );
	ack[3] = std::move( setpoints[3].ApplyCorrection<std::plus<double>>(corr_u)  );

	ack[4] = std::move( setpoints[4].ApplyCorrection<std::minus<double>>(corr_v) );
	ack[5] = std::move( setpoints[5].ApplyCorrection<std::plus<double>>(corr_v)  );
	ack[6] = std::move( setpoints[6].ApplyCorrection<std::minus<double>>(corr_u) );
	ack[7] = std::move( setpoints[7].ApplyCorrection<std::plus<double>>(corr_u)  );

#ifdef WAIT_FOR_TX_ACK
	wait_for_tx_ack(fName, ack);
#endif
}
