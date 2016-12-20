#pragma once

// Access to a member.
enum Access {
	aPublic,
	aProtected,
	aPrivate
};

// Output
wostream &operator <<(wostream &to, Access a);
