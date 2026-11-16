#include "QwFeedbackCorrectionPayload.h"
#include <string>
#include "QwLog.h"
#include "QwErrorHandling.h"

std::string_view stringify(FbCorrType type)
{
	std::string_view sv{};
	switch(type) {
	case FbCorrType::ASYM:
		sv = "ASYM";
		break;
	case FbCorrType::DIFFX:
		sv = "DIFFX";
		break;
	case FbCorrType::DIFFY:
		sv = "DIFFX";
		break;
	default:
		QwWarning << "Unknown enum GetFbCorrType" << '\n';
		break;
	}
	return sv;
}

FbCorrType GetFbCorrType(std::string_view sv)
{
	FbCorrType type;
	if(sv == "ASYM") {
		type = FbCorrType::ASYM;
	} else if(sv == "DIFFX") {
		type = FbCorrType::DIFFX;
	} else if(sv == "DIFFY") {
		type = FbCorrType::DIFFY;
	} else {
		QwWarning << "Unknown enum GetFbCorrType" << '\n';
	}
	return type;
}

void QwFeedbackCorrectionPayload::SetImpl(int index, const double val)
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	fValues[index] = val;
}
double QwFeedbackCorrectionPayload::GetImpl(int index) const
{
	assert(index < int(ARR_SIZE) && "Enum index out of array bounds!");
	return fValues[index];
}

std::unique_ptr<VQwFeedbackValues> QwFeedbackCorrectionPayload::Clone() const
{
	return std::make_unique<QwFeedbackCorrectionPayload>(*this);
}
