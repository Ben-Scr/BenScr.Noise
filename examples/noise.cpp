#include <benscr/noise/generator.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    namespace Noise = BenScr::Noise;
    const Noise::Generator generator(42);
    std::array<double, 32> heights{};
    const Noise::Grid2D grid{8, 4, -0.5, -0.25, 0.05, 0.05};
    generator.FillFractal2D(heights, grid, {4, 1.0, 2.0, 0.5});
    for (double height : heights)
    {
        if (!std::isfinite(height) || height < -1 || height > 1)
        {
            throw std::runtime_error("Invalid field sample.");
        }
    }
    if (heights.front() != generator.SampleFractal2D(-0.5, -0.25, {4, 1.0, 2.0, 0.5}) ||
        std::abs(generator.Sample3D(-1, 2, -3) - 0.5360976881195421) > 1e-14)
    {
        throw std::runtime_error("Noise reference check failed.");
    }
    std::cout << "Noise field verified.\n";
}
