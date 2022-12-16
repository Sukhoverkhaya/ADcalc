#pragma once

struct ST
{
public:
    bool stateChanged;
	bool ON;

	ST(){stateChanged = false; ON = false;}
	virtual void On()  { ON = true; }
	virtual void Off() { ON = false; }
};