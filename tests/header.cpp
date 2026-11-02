#include <benscr/noise/generator.hpp>

int main()
{
    BenScr::Noise::Generator generator(42);
    return generator.GetSeed() == 42 ? 0 : 1;
}
