#pragma once

//Inherit from this object (as private) to make the object non-copyable.
//Also gives you a virtual destructor.
class NoCopy {
public:
	NoCopy();
	virtual ~NoCopy();
private:
	NoCopy(const NoCopy &);
	NoCopy &operator =(const NoCopy &);
};

//Creates a singleton class with a protected ctor.
class Singleton : NoCopy {
protected:
	Singleton();
};