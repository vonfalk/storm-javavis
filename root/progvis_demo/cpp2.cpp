struct elem {
	elem *next;
	int value;
};

elem *createList(int count) {
	elem *head;
	if (count <= 0)
		return head;

	head = new elem;
	head->value = 1;
	elem *tail = head;
	while (tail->value < count) {
		elem *tmp = new elem;
		tail->next = tmp;
		tmp->value = tail->value + 1;
		tail = tmp;
	}

	return head;
}

int main() {
	elem *list = createList(4);

	return 0;
}
