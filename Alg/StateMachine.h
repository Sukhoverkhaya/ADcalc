#pragma once

struct ST
{
public:

    bool stateChanged;
	bool ON;

	void On()  { ON = true; }
	void Off() { ON = false; }
};