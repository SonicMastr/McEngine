#ifdef __vita__

#ifndef VITATIMER_H
#define VITATIMER_H

#include "Timer.h"

#include <time.h>

class VitaTimer : public BaseTimer
{
public:
	VitaTimer();
    virtual ~VitaTimer() {;}

	virtual void start();
	virtual void update();

	virtual inline double getDelta() const {return m_delta;}
	virtual inline double getElapsedTime()  const {return m_elapsedTime;}
    virtual inline uint64_t getElapsedTimeMS() const override {return m_elapsedTimeMS;}

private:
    timespec m_startTime;
    timespec m_currentTime;

    double m_delta;
    double m_elapsedTime;
    uint64_t m_elapsedTimeMS;
};

#endif

#endif