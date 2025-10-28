#include <iostream>

#include "Vector.h"

int main()
{
    Vector<int> test;

    std::cout << "size: " << test.size() << std::endl;
    test.push_back(1);
    std::cout << "size: " << test.size() << std::endl;
    test.push_back(4);
    std::cout << "size: " << test.size() << std::endl;
    test.push_back(3);
    std::cout << "size: " << test.size() << std::endl;
    test.push_back(2);
    std::cout << "size: " << test.size() << std::endl;
    test.push_back(1);
    std::cout << "size: " << test.size() << std::endl;
    test.push_back(0);
    std::cout << "size: " << test.size() << std::endl;

    test.pop_back();
    std::cout << "size after pop: " << test.size() << std::endl;
    test.pop_back();
    std::cout << "size after pop: " << test.size() << std::endl;

    test[0] = 99;
    test[1] = 67;
    for (int i : test)
    {
        std::cout << i << std::endl;
    }

    test.clear();
    std::cout << "Size after clear: " << test.size() << std::endl;
    test.pop_back();
    std::cout << "size after pop: " << test.size() << std::endl;
    return 0;
}