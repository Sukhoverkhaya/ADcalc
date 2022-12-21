#pragma once

struct SmartRoundBuff
{
	SmartRoundBuff(s32 size)
	:  rail(size)
		,buf( new ToneTuple[size] )
		,forward(0)
		,backward(0)
	{ }
	
	const s32 rail;
	ToneTuple* buf;
	s32 forward;   //rd
	s32 backward;  //wr
		
};

struct RBIterator
{
	RBIterator(SmartRoundBuff& _buf, s32 _it = 0)
	:	 buf(_buf)
		,it(_it)
	{ }
	
	RBIterator(const RBIterator& rhs)
	: buf(rhs.buf)
	{
		this->it = rhs.it;
	}
	
	void operator--()
	{
		--it;
		if( it < 0 ) it = buf.rail + it; //NOTE(romanm): в теории нужно it %= rail;
	}
	
	void operator++()
	{
		++it %= buf.rail;
	}
	
	void Set(s32 env, s32 tone, s32 press)
	{
		buf.buf[it].env   = env;
		buf.buf[it].tone  = tone;
		buf.buf[it].press = press;
	}
	
	s32& Get()
	{
		return buf.buf[it].env;
	}
	
	s32& GetTone()
	{
		return buf.buf[it].tone;
	}	
	
	s32& GetPress()
	{
		return buf.buf[it].press;
	}
	
	
	bool operator!=(const RBIterator& rhs)
	{
		return this->it != rhs.it;
	}	
	
	void operator=(const RBIterator& rhs)
	{
		this->it = rhs.it;
	}
	
	void operator=(s32 _it)
	{
		if( _it < 0) it = buf.rail + _it; //NOTE(romanm): в теории нужно it %= rail;
		else         it = _it % buf.rail; 
	}	
	
	SmartRoundBuff& buf;
	s32 it;
};