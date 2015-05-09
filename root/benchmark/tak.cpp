#include <iostream>
using namespace std;

int tak(int x, int y, int z) {
	if (y < x) {
		return tak(
			tak(x - 1, y, z),
			tak(y - 1, z, x),
			tak(z - 1, x, y)
			);
	} else {
		return z;
	}
}

int main() {
	// Takes ~27s (including compile)
	// 15s (gcc, o3) 0.3s compile
	for (int i = 0; i < 13; i++) {
		cout << "Tak " << i << endl;
		cout << "=> " << tak(i, 0, -i) << endl;
	}

	return 0;
}
