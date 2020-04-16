struct account {
	int balance;
};

int num_accounts;
struct account *accounts;

bool transfer(int amount, int from, int to) {
	bool ok = accounts[from].balance >= amount;
	if (ok) {
		accounts[from].balance -= amount;
		accounts[to].balance += amount;
	}

	return ok;
}

int main(void) {
	NO_STEP {
		num_accounts = 4;
		accounts = malloc(sizeof(struct account) * num_accounts);
		for (int i = 0; i < num_accounts; i++) {
			accounts[i].balance = 10;
		}
	}

	transfer(5, 1, 2);


	return 0;
}
