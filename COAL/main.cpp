#include <iostream>

extern "C" int check_moisture(int moisture);

int main()
{
    int moisture;

    std::cout << "Enter soil moisture value: ";
    std::cin >> moisture;

    int result = check_moisture(moisture);

    if (result == 1)
        std::cout << "Soil moisture is LOW." << std::endl;
    else
        std::cout << "Soil moisture is OK." << std::endl;

    return 0;
}