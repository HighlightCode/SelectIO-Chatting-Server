#include <iostream>
#include <thread>
#include "RunProcess.h"

int main()
{
	RunProcess process;
	process.Init();

	std::thread logicThread([&]() {
		process.Run(); }
	);

	std::cout << "press any key to exit...";
	getchar();

	process.Stop();
	logicThread.join();

	return 0;
}