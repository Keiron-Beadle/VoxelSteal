#pragma once
#include "../../Headers/gil.h"
#include "../../Headers/Entity.h"
#include "../../Headers/Timer.h"
#undef max

class System {
private:
	std::function<void()> _processFunc;
	std::thread _thread;
	std::unique_ptr<Timer> _timer;
	SK::UINT32 _core, _freq;
	std::atomic<SK::UINT32> _actFreq;
	SK::BOOL _started = false;

protected:
	static std::vector<SK::UINT32> CoreRecord;
	Addressee Address;

	std::mutex _entityLock;

	std::atomic<SK::BOOL> _shouldJoin{ false };
	std::atomic<SK::BOOL> _shouldChangeCore{ false };
	std::atomic<SK::BOOL> _pullNewEntities{false};

	nodconst std::unique_ptr<Timer>& GetTimer() const { return _timer; }
	nodconst SK::FLOAT dT() const { return _timer->dT(); }

	void SetFrequency(const SK::UINT32 freq);
	void SetCore(const SK::UINT32 core);
	const SK::UINT32 GetLeastUtilisedCore() const;
public:
	System(SK::UINT32 freq = 60, 
		SK::UINT32 reqCore = std::numeric_limits<SK::UINT32>::max());
	virtual ~System();

	nodconst SK::UINT32 GetActualFrequency() const { return _actFreq.load(); }

	virtual void AddEntity(std::shared_ptr<Entity> e) = 0;
	virtual void UpdateEntity(std::shared_ptr<Entity> e) = 0;

	virtual void OnMessage(std::shared_ptr<Message> msg) = 0;
	virtual void StartSystem();
	virtual void CancelSystem();

protected:
	virtual void MoveToCore();

private:
	virtual void Process() = 0;
	void ThreadFunc();

};