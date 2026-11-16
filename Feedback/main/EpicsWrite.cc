#include "QwEpicHandler.h"
#include "QwEpicChannel.h"
#include "QwLog.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	QwMessage << "EPICS Write Test\n";
	auto& epics = EpicHandler::Instance();

	auto ioc = epics.ConnectChannel("IGL1I00DI24_24M");
	auto hv = epics.ConnectChannel("RTPXPORT1Q1calc.A");
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	epics.GetStatus(1);
	epics.CheckConnection();
	double value[] = {5000, 0};
	int counter = 0;
	do {
		std::cout << "Press 'q' to quit...\n";
		auto new_val = value[counter % 2];
		auto start = std::chrono::high_resolution_clock::now();
		auto put_fut = hv->PutAsync(new_val);
		put_fut.get();
		auto get_fut = hv->GetAsync<double>();
		auto val = get_fut.get();
		auto end = std::chrono::high_resolution_clock::now();
		auto dur_ns = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
		QwMessage << "Write/Read Timing: Received (" << val << ") in (" << dur_ns << ") [us]" << QwLog::endl;
		counter++;

	} while(getchar() != 'q');

	return 0;
}
