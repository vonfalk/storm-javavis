public class HelloWorld {

  void test() {
    print("This you should see.");
    return;
    print("You should not see this.");
  }

  int test2(int x) {
    return x + 20;
  }

  int test3() {
    return;
  }

  void test4() {
    return 20;
  }

    public static void main() {
      test();

      int i;
      i = test2(20);

      //test3();
      //i = test4();
    }
}