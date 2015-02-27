#pragma once

/**
 * A simple lock.
 */
class Lock {
public:
	// Create.
	Lock();

	// Destroy.
	~Lock();

	// Lock the lock, and make sure it is automatically unlocked at the end of the scope.
	class L {
	public:
		L(Lock &l);
		L(Lock *l);
		~L();
	private:
		Lock *l;
	};

private:
	// Underlying implementation.
	CRITICAL_SECTION cs;
};
