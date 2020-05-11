public class HelloWorld {
    public static void main(String[] args) {
      boolean b;
      b = true;
      int i;
      i = 20;
      if (b) {
        print("True!");
        b = false;
      }

      while (b) {
        print("You should not see this.");
      }
    }
}
