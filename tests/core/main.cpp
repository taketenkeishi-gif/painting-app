#include <exception>
#include <iostream>

void runDocumentTests();
void runRendererTests();
void runLayerTests();
void runBrushToolTests();

int main() {
  try {
    runDocumentTests();
    runRendererTests();
    runLayerTests();
    runBrushToolTests();
    std::cout << "All core tests passed.\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Core tests failed: " << ex.what() << '\n';
    return 1;
  }
}
