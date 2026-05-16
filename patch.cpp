#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::ifstream in("src/interpreter/interpreter.cpp");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t pos = content.find("auto* input = net_.find_input(name);");
    if (pos != std::string::npos) {
        std::string inj = "log(LogType::INFO, \"DEBUG: checking name='\" + name + \"' against net_.inputs[0]='\" + (net_.inputs.empty() ? \"EMPTY\" : net_.inputs[0].name) + \"'\");\n    ";
        content.insert(pos, inj);
        std::ofstream out("src/interpreter/interpreter.cpp");
        out << content;
        out.close();
        std::cout << "Patched successfully.\n";
    }
    return 0;
}
