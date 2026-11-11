#include "QwEpicHandler.h"
#include "QwEpicChannel.h"
#include "QwLog.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
	QwMessage << "Hello World\n";
	auto& epics = EpicHandler::Instance();
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	epics.GetStatus(1);
	do {
		epics.CheckConnection();
		std::cout << "Press 'q' to quit...\n";
	} while(getchar() != 'q');

	return 0;
}
