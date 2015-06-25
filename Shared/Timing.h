#pragma once
#include "EnginePtr.h"

namespace storm {
	STORM_PKG(core);

	class Str;

	/**
	 * Basic time api for measuring short intervals.
	 */

	class Moment {
		STORM_VALUE;
	public:
		// Create a representation of 'now'
		STORM_CTOR Moment();

		// Inernal use, create with specific value.
		inline Moment(int64 v) : v(v) {}

		// The time value, measured in us from some point in time.
		int64 v;
	};

	class Duration {
		STORM_VALUE;
	public:
		// Zero duration.
		inline STORM_CTOR Duration() : v(0) {}

		// Create a duration with a time in us.
		inline Duration(int64 t) : v(t) {}

		// The actual value, in us.
		int64 v;

		// Get the value in various units.
		inline Int STORM_FN asUs() { return Int(v); }
		inline Int STORM_FN asMs() { return Int(v / 1000); }
		inline Int STORM_FN asS() { return Int(v / 1000000); }
	};

	// Create durations in various units. TODO: Larger type for parameter?
	Duration STORM_FN h(Int v);
	Duration STORM_FN min(Int v);
	Duration STORM_FN s(Int v);
	Duration STORM_FN ms(Int v);
	Duration STORM_FN us(Int v);

	// Output.
	wostream &operator <<(wostream &to, Moment m);
	wostream &operator <<(wostream &to, Duration d);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Moment m);
	Str *STORM_ENGINE_FN toS(EnginePtr e, Duration d);

	// Arithmetic operations between durations and moments.
	inline Duration STORM_FN operator -(Moment a, Moment b) { return Duration(a.v - b.v); }
	inline Moment STORM_FN operator +(Moment a, Duration b) { return Moment(a.v + b.v); }
	inline Moment STORM_FN operator +(Duration b, Moment a) { return Moment(a.v + b.v); }
	inline Moment STORM_FN operator -(Moment a, Duration b) { return Moment(a.v - b.v); }
	inline Moment &STORM_FN operator +=(Moment &a, Duration b) { a.v += b.v; return a; }
	inline Moment &STORM_FN operator -=(Moment &a, Duration b) { a.v -= b.v; return a; }

	inline Duration STORM_FN operator +(Duration a, Duration b) { return Duration(a.v + b.v); }
	inline Duration STORM_FN operator -(Duration a, Duration b) { return Duration(a.v - b.v); }
	inline Duration STORM_FN operator *(Duration a, Int factor) { return Duration(a.v * factor); }
	inline Duration STORM_FN operator /(Duration a, Int factor) { return Duration(a.v / factor); }
	inline Int STORM_FN operator /(Duration a, Duration b) { return int(a.v / b.v); }
	inline Duration STORM_FN operator %(Duration a, Duration b) { return Duration(a.v % b.v); }
	inline Duration &STORM_FN operator +=(Duration &a, Duration b) { a.v += b.v; return a; }
	inline Duration &STORM_FN operator -=(Duration &a, Duration b) { a.v -= b.v; return a; }
	inline Duration &STORM_FN operator *=(Duration &a, Int b) { a.v *= b; return a; }
	inline Duration &STORM_FN operator /=(Duration &a, Int b) { a.v /= b; return a; }

	// Comparisons. TODO: Consider wrapping of Moments.
	inline Bool STORM_FN operator ==(Moment a, Moment b) { return a.v == b.v; }
	inline Bool STORM_FN operator !=(Moment a, Moment b) { return a.v != b.v; }
	inline Bool STORM_FN operator >(Moment a, Moment b) { return a.v > b.v; }
	inline Bool STORM_FN operator <(Moment a, Moment b) { return a.v < b.v; }
	inline Bool STORM_FN operator >=(Moment a, Moment b) { return a.v >= b.v; }
	inline Bool STORM_FN operator <=(Moment a, Moment b) { return a.v <= b.v; }

	inline Bool STORM_FN operator ==(Duration a, Duration b) { return a.v == b.v; }
	inline Bool STORM_FN operator !=(Duration a, Duration b) { return a.v != b.v; }
	inline Bool STORM_FN operator <(Duration a, Duration b) { return a.v < b.v; }
	inline Bool STORM_FN operator >(Duration a, Duration b) { return a.v > b.v; }
	inline Bool STORM_FN operator <=(Duration a, Duration b) { return a.v <= b.v; }
	inline Bool STORM_FN operator >=(Duration a, Duration b) { return a.v >= b.v; }

}
