#include "QwEpicHandler.h"
#include "QwEpicChannel.h"
#include "QwIHWP.h"
#include "QwLog.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	QwMessage << "Reading IHWP Status\n";
	auto& epics = EpicHandler::Instance();
	auto ihwp   = epics.ConnectChannel(IHWP_IOC::IHWP_PV);
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	epics.GetStatus(1);
	epics.CheckConnection();

	auto start = std::chrono::high_resolution_clock::now();
	auto fut = ihwp->GetAsync<std::string>();
	auto val = fut.get();
	auto end = std::chrono::high_resolution_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
	QwMessage << "Received (" << val << ") in (" << dur << ") [us]" << QwLog::endl;


	return 0;
}
