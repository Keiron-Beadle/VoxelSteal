#include <iostream>
#include "../Headers/System.h"
#include <windows.h>
#undef max
#undef min

std::vector<SK::UINT32> System::CoreRecord;

System::System(SK::UINT32 freq, SK::UINT32 reqCore) 
	: _core(reqCore), _freq(freq), Address(Addressee::ALL) {

	//Yes it's naive but that's my specialty. 
	if (reqCore == std::numeric_limits<SK::UINT32>::max() ||
		reqCore < 0 || reqCore > SK_CPU_CORE_COUNT) 
	{
		_core = GetLeastUtilisedCore();
	}
	CoreRecord.push_back(_core);

	_timer = std::make_unique<Timer>(freq);

}

System::~System() {
	std::cout << "System :: Destructor called" << std::endl;
	if (!_shouldJoin) {
		_shouldJoin = true;
		std::cout << "System :: Called for Process thread join." << std::endl;
		_thread.join();
		std::cout << "System :: Process thread joined." << std::endl;
	}
}

const SK::UINT32 System::GetLeastUtilisedCore() const
{
	const SK::UINT32 cores = SK_CPU_CORE_COUNT;
	std::vector<SK::UINT32> usageVector(cores, 0);
	for (const auto& core : CoreRecord)
		usageVector[core]++;

	SK::UINT32 leastUsedCore = 0;
	auto leastUsageCount = static_cast<SK::UINT32>(CoreRecord.size());

	for (SK::UINT32 core = 0; core < cores; ++core) {
		if (usageVector[core] < leastUsageCount) {
			leastUsageCount = usageVector[core];
			leastUsedCore = core;
		}
	}

	return leastUsedCore;
}

void System::ThreadFunc()
{
	SetCore(_core);
	MoveToCore();

	while (!_shouldJoin.load()) {
		_timer->Tick();
		_actFreq.store(_timer->FPS());
		_processFunc();
		if (_shouldChangeCore.load()) {
			std::cout << "Moved core.\n";
			MoveToCore();
		}

		_timer->WaitForInterval();
	}
}

void System::StartSystem()
{
	if (_started) return;
	_processFunc = [this]() {this->Process(); };
	_thread = std::thread(&System::ThreadFunc, this);
	_started = true;
}

void System::CancelSystem()
{
	_shouldJoin = true;
	_thread.join();
	_started = false;
}

void System::SetFrequency(const SK::UINT32 freq)
{
	_freq = freq;
	_timer->SetFrequency(static_cast<SK::DOUBLE>(freq));
}

void System::SetCore(const SK::UINT32 core)
{
	_core = core;
	_shouldChangeCore = true;
}

void System::MoveToCore()
{
	//OutputDebugString(L"Move To Core\n");
	if (!_started) return;
	HANDLE h = _thread.native_handle();
	DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << _core;
	if (!SetThreadAffinityMask(h, affinityMask))
		std::cerr << "System :: Failed to move system to new core\n";
	_shouldChangeCore = false;
	//OutputDebugString(L"Finished Move\n");
}