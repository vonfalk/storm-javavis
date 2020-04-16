struct account {
	int balance;
};

int num_accounts;
struct account *accounts;

bool transfer(int amount, struct account *from, struct account *to) {
	bool ok = from->balance >= amount;
	if (ok) {
		from->balance -= amount;
		to->balance += amount;
	}

	return ok;
}

void worker(int amount, int from, int to) {
	if (!transfer(amount, &accounts[from], &accounts[to]))
		printf("Transfer %d from %d to %d failed!\n", amount, from, to);
}

int main(void) {
	NO_STEP {
		num_accounts = 10;
		accounts = malloc(sizeof(struct account) * num_accounts);
		for (int i = 0; i < num_accounts; i++) {
			accounts[i].balance = 10;
		}
	}

	thread_new(&worker, 10, 3, 4);
	thread_new(&worker, 10, 1, 2);
	worker(10, 1, 2);

	return 0;
}
