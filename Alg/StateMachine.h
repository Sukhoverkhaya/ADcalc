#pragma once

struct ST
{
public:
    bool stateChanged;
	bool ON;

	virtual void On()  { ON = true; }
	virtual void Off() { ON = false; }
};