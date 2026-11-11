#include "QwEpicHandler.h"
#include "QwEpicChannel.h"
#include "QwLog.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	QwMessage << "EPICS Read Test\n";
	auto& epics = EpicHandler::Instance();
	auto hv = epics.ConnectChannel("RTPXPORT1Q1calc.A");
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	epics.GetStatus(1);
	epics.CheckConnection();
	do {
		std::cout << "Press 'q' to quit...\n";
		auto start = std::chrono::high_resolution_clock::now();
		auto fut = hv->GetAsync<double>();
		auto val = fut.get();
		auto end = std::chrono::high_resolution_clock::now();
		auto dur_ns = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
		QwMessage << "Received (" << val << ") in (" << dur_ns << ") [us]" << QwLog::endl;

	} while(getchar() != 'q');

	return 0;
}
