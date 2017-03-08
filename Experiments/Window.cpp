#pragma once

// Compile with 'cl /d1reportAllClassLayout alignment.cpp'
// to see struct layout. Also see /d1reportSingleClassLayoutXxx
// where Xxx is matched agains the class name

typedef float Float;
typedef bool Bool;
typedef unsigned long long Word;

class Str;
class Thread;

class RootObject {
public:
	virtual ~RootObject();
};

class TObject : public RootObject {
public:
	Thread *t;
};

class Handle {
public:
	size_t v;
};

class Point {
public:
	Float x;
	Float y;
};

class Rect {
public:
	Point p0;
	Point p1;
};

class Frame;
class Container;

class Window : public TObject {
public:
	Handle handle;
	Container *parent;
	Frame *root;
	Bool visible;
	Str *text;
	Rect pos;
	Duration interval;
};

class Map;
class Event;

class Container : public Window {
public:
	Map *ids;
	Map *windows;
	Nat lastId;
	Event *onClose;
};

class Frame : public Container {
public:
	Long style;
	Rect rect;
	Bool full;
	Bool cursor;
};
