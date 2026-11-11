#pragma once
#include <atomic>
#include "QwEpicHandler.h"
#include "QwEpicChannel.h"
#include "QwObserver.h"
#include "QwPublisher.h"
#include "QwFeedbackLogger.h"
#include "QwAtomicEpicTypes.h"
enum class IHWP {
    kIN  = 0, // PER EPICS
    kOUT = 1  // PER EPICS                                                                                
};
std::string stringify(IHWP ihwp);


/**
 * @class IHWP_IOC
 * @brief IHWP IOC container
 * @details
 * 		  Instead of requesting (and waiting) for up-to-date EPICS values every time
 * 		  we need them, we can let the EPICS lib handle the updates for us. This is
 * 		  achieved using the Observer pattern and the EPICS lib callback functionality.
 * 		  When a IOC value changes, the EPICS lib updates our held value atomically.
 *
 * 		  Logging is handled in a similar manner. A logger (observer) is attached to the
 * 		  setpoint (publisher). Everytime we request or receive a change in the setpoint value,
 * 		  we notify the logger of the change.
 */
class IHWP_IOC : public EPICSObserver<int>, public Publisher<QwFeedbackLogPayload>
{
	EpicChannel* fChannel;
	AtomicEpicsType<int> fCurrState;
public:
	static const char* IHWP_PV;
	static constexpr std::size_t fNumPossibleStates{2};
	void Update(int const& data) override;
    IHWP GetState() const;
public:
	IHWP_IOC();
	IHWP_IOC(EpicChannel* channel);
	~IHWP_IOC();
	IHWP_IOC(IHWP_IOC const& other);
	IHWP_IOC(IHWP_IOC&& other) = delete;
	IHWP_IOC& operator=(IHWP_IOC const& other) = delete;
	IHWP_IOC& operator=(IHWP_IOC&& other) = delete;
};    
