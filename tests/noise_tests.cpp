#include <benscr/noise/generator.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace Noise = BenScr::Noise;

namespace
{
int cases = 0;

void Check(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void Near(double actual, double expected, double tolerance = 1e-14)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
    {
        std::cerr << std::setprecision(17) << "Actual: " << actual << ", expected: " << expected
                  << ", absolute tolerance: " << tolerance << '\n';
        throw std::runtime_error("Reference mismatch.");
    }
}

template <typename Exception, typename Action> void Throws(Action action)
{
    try
    {
        action();
    }
    catch (const Exception &)
    {
        return;
    }
    throw std::runtime_error("Expected exception was not thrown.");
}

template <typename Action> void Case(const char *name, Action action)
{
    action();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
} // namespace

int main()
{
    try
    {
        const Noise::Generator noise(42);
        Case("seed and algorithm version",
             [&]
             {
                 Check(noise.GetSeed() == 42 && Noise::kAlgorithmVersion == 1,
                       "Identity mismatch.");
                 Check(Noise::Generator{}.GetSeed() == 0, "Default seed mismatch.");
             });
        Case(
            "independent integer hash vectors",
            [&]
            {
                // Values were derived with arbitrary-precision integer arithmetic modulo 2^64.
                Near(Noise::Generator{}.Sample3D(0, 0, 0), 0.7666216164272852, 0);
                Near(noise.Sample3D(0, 0, 0), 0.4831297575436466, 0);
                Near(noise.Sample3D(1, 0, 0), 0.30743147797410897, 0);
                Near(noise.Sample3D(-1, 2, -3), 0.5360976881195421, 0);
                Near(
                    Noise::Generator(std::numeric_limits<std::uint64_t>::max()).Sample2D(1e9, -1e9),
                    -0.0897639550194056, 0);
            });
        Case("half-cell interpolation averages corners",
             [&]
             {
                 Near(noise.Sample2D(0.5, 0.5), 0.2849549233317711);
                 Near(noise.Sample3D(0.5, 0.5, 0.5), -0.10635038160140473);
             });
        Case("quarter-cell quintic weight",
             [&]
             {
                 constexpr double kWeight = 0.103515625;
                 Near(noise.Sample2D(0.25, 0),
                      (1.0 - kWeight) * 0.4831297575436466 + kWeight * 0.30743147797410897);
             });
        Case("negative cells use floor",
             [&]
             {
                 const double expected = (noise.Sample2D(-1, -1) + noise.Sample2D(0, -1) +
                                          noise.Sample2D(-1, 0) + noise.Sample2D(0, 0)) /
                                         4.0;
                 Near(noise.Sample2D(-0.5, -0.5), expected);
             });
        Case("two-dimensional slice contract",
             [&]
             {
                 for (int i = -40; i <= 40; ++i)
                 {
                     Near(noise.Sample2D(i * 0.123, -i * 0.071),
                          noise.Sample3D(i * 0.123, -i * 0.071, 0), 0);
                 }
             });
        Case("bounds over mixed signs",
             [&]
             {
                 for (int x = -30; x <= 30; ++x)
                 {
                     for (int y = -30; y <= 30; ++y)
                     {
                         const double value =
                             noise.Sample3D(static_cast<double>(x) * 0.173,
                                            static_cast<double>(y) * 0.317, (x - y) * 0.19);
                         Check(value >= -1.0 && value <= 1.0, "Noise escaped bounds.");
                     }
                 }
             });
        Case("cell boundaries are continuous",
             [&]
             {
                 for (int x = -8; x <= 8; ++x)
                 {
                     Near(noise.Sample3D(x - 1e-5, 0.37, -0.29),
                          noise.Sample3D(x + 1e-5, 0.37, -0.29), 1e-9);
                 }
             });
        Case("coordinate bounds inclusive",
             [&]
             {
                 Check(std::isfinite(noise.Sample3D(-1e9, 1e9, -1e9)), "Valid boundary rejected.");
                 Throws<std::invalid_argument>([&] { (void)noise.Sample2D(1e9 + 1, 0); });
                 Throws<std::invalid_argument>([&] { (void)noise.Sample2D(-1e9 - 1, 0); });
             });
        Case("nonfinite and extreme coordinates rejected",
             [&]
             {
                 for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                        std::numeric_limits<double>::infinity(),
                                        -std::numeric_limits<double>::infinity(),
                                        std::numeric_limits<double>::max()})
                 {
                     Throws<std::invalid_argument>([&] { (void)noise.Sample2D(invalid, 0); });
                     Throws<std::invalid_argument>([&] { (void)noise.Sample2D(0, invalid); });
                     Throws<std::invalid_argument>([&] { (void)noise.Sample3D(0, 0, invalid); });
                 }
             });
        Case("one octave matches scaled noise",
             [&]
             {
                 const Noise::FractalSettings settings{1, 2.5, 3.0, 0.75};
                 Near(noise.SampleFractal2D(0.25, -0.5, settings), noise.Sample2D(0.625, -1.25), 0);
                 Near(noise.SampleFractal3D(0.25, -0.5, 0.75, settings),
                      noise.Sample3D(0.625, -1.25, 1.875), 0);
             });
        Case("fractal normalized weighted sum",
             [&]
             {
                 const Noise::FractalSettings settings{3, 1.0, 2.0, 0.5};
                 const double expected =
                     (noise.Sample2D(0.25, -0.75) + 0.5 * noise.Sample2D(0.5, -1.5) +
                      0.25 * noise.Sample2D(1, -3)) /
                     1.75;
                 Near(noise.SampleFractal2D(0.25, -0.75, settings), expected);
             });
        Case("zero gain and unit lacunarity",
             [&]
             {
                 Near(noise.SampleFractal2D(0.37, -0.61, {5, 1, 2, 0}), noise.Sample2D(0.37, -0.61),
                      0);
                 Near(noise.SampleFractal3D(0.37, -0.61, 0.23, {32, 1, 1, 1}),
                      noise.Sample3D(0.37, -0.61, 0.23));
             });
        Case("fractal bounds and negative inputs",
             [&]
             {
                 for (int i = -100; i <= 100; ++i)
                 {
                     const double value = noise.SampleFractal3D(i * 0.031, i * -0.079, i * 0.053);
                     Check(value >= -1 && value <= 1, "Fractal escaped bounds.");
                 }
             });
        Case("invalid octave and gain settings",
             [&]
             {
                 for (int octaves : {0, -1, 33})
                 {
                     Throws<std::invalid_argument>(
                         [&] { (void)noise.SampleFractal2D(0, 0, {octaves}); });
                 }
                 for (double gain : {-0.01, 1.01, std::numeric_limits<double>::quiet_NaN()})
                 {
                     Throws<std::invalid_argument>(
                         [&] { (void)noise.SampleFractal2D(0, 0, {2, 1, 2, gain}); });
                 }
             });
        Case("invalid frequency and lacunarity",
             [&]
             {
                 for (double frequency : {0.0, -1.0, std::numeric_limits<double>::infinity()})
                 {
                     Throws<std::invalid_argument>(
                         [&] { (void)noise.SampleFractal3D(0, 0, 0, {2, frequency}); });
                 }
                 for (double lacunarity : {0.0, 0.5, std::numeric_limits<double>::infinity()})
                 {
                     Throws<std::invalid_argument>(
                         [&] { (void)noise.SampleFractal3D(0, 0, 0, {2, 1, lacunarity}); });
                 }
             });
        Case("octave frequency overflow and scaled coordinates",
             [&]
             {
                 Throws<std::invalid_argument>(
                     [&]
                     {
                         (void)noise.SampleFractal2D(0, 0,
                                                     {3, 1, std::numeric_limits<double>::max(), 0});
                     });
                 Throws<std::invalid_argument>([&] { (void)noise.SampleFractal2D(1e9, 0, {2}); });
                 Throws<std::invalid_argument>([&]
                                               { (void)noise.SampleFractal3D(0, 0, -1e9, {2}); });
             });
        Case("bulk2D row order and negative steps",
             [&]
             {
                 const Noise::Grid2D grid{7, 5, 0.7, -0.9, -0.17, 0.21};
                 std::array<double, 35> field{};
                 noise.Fill2D(field, grid);
                 for (std::size_t y = 0; y < grid.Height; ++y)
                 {
                     for (std::size_t x = 0; x < grid.Width; ++x)
                     {
                         Near(field[y * grid.Width + x],
                              noise.Sample2D(grid.OriginX + static_cast<double>(x) * grid.StepX,
                                             grid.OriginY + static_cast<double>(y) * grid.StepY));
                     }
                 }
             });
        Case("bulk3D row order",
             [&]
             {
                 const Noise::Grid3D grid{3, 4, 2, -0.7, 0.9, -1.2, 0.17, -0.21, 0.31};
                 std::array<double, 24> field{};
                 noise.Fill3D(field, grid);
                 for (std::size_t z = 0; z < grid.Depth; ++z)
                 {
                     for (std::size_t y = 0; y < grid.Height; ++y)
                     {
                         for (std::size_t x = 0; x < grid.Width; ++x)
                         {
                             Near(field[(z * grid.Height + y) * grid.Width + x],
                                  noise.Sample3D(grid.OriginX + static_cast<double>(x) * grid.StepX,
                                                 grid.OriginY + static_cast<double>(y) * grid.StepY,
                                                 grid.OriginZ +
                                                     static_cast<double>(z) * grid.StepZ));
                         }
                     }
                 }
             });
        Case("bulk fractal2D",
             [&]
             {
                 std::array<double, 12> field{};
                 const Noise::Grid2D grid{4, 3, -0.1, 0.2, 0.1, -0.2};
                 const Noise::FractalSettings settings{4, 0.5, 3, 0.7};
                 noise.FillFractal2D(field, grid, settings);
                 for (std::size_t y = 0; y < 3; ++y)
                 {
                     for (std::size_t x = 0; x < 4; ++x)
                     {
                         Near(field[y * 4 + x],
                              noise.SampleFractal2D(-0.1 + static_cast<double>(x) * 0.1,
                                                    0.2 - static_cast<double>(y) * 0.2, settings));
                     }
                 }
             });
        Case("bulk fractal3D",
             [&]
             {
                 std::array<double, 8> field{};
                 const Noise::Grid3D grid{2, 2, 2, 0, 0, 0, 0.1, 0.2, -0.3};
                 noise.FillFractal3D(field, grid);
                 for (std::size_t z = 0; z < 2; ++z)
                 {
                     for (std::size_t y = 0; y < 2; ++y)
                     {
                         for (std::size_t x = 0; x < 2; ++x)
                         {
                             Near(field[(z * 2 + y) * 2 + x],
                                  noise.SampleFractal3D(static_cast<double>(x) * 0.1,
                                                        static_cast<double>(y) * 0.2,
                                                        static_cast<double>(z) * -0.3));
                         }
                     }
                 }
             });
        Case("zero steps produce a constant field",
             [&]
             {
                 std::array<double, 4> field{};
                 noise.Fill2D(field, {2, 2, 0.5, -0.5, 0, 0});
                 for (double value : field)
                 {
                     Near(value, noise.Sample2D(0.5, -0.5), 0);
                 }
             });
        Case("empty grids",
             [&]
             {
                 noise.Fill2D({}, {0, 5});
                 noise.Fill2D({}, {5, 0});
                 noise.Fill3D({}, {5, 3, 0});
                 noise.Fill3D({}, {5, 0, 3});
                 noise.FillFractal2D({}, {});
                 noise.FillFractal3D({}, {});
             });
        Case("span sizes must match exactly",
             [&]
             {
                 std::array<double, 3> field{9, 9, 9};
                 Throws<std::invalid_argument>([&] { noise.Fill2D(field, {2, 2}); });
                 Throws<std::invalid_argument>([&] { noise.Fill3D(field, {1, 1, 2}); });
                 Throws<std::invalid_argument>([&] { noise.FillFractal2D(field, {}); });
                 Check(field == std::array<double, 3>{9, 9, 9}, "Failed fill changed output.");
             });
        Case("dimension and product limits",
             [&]
             {
                 Throws<std::length_error>([&]
                                           { noise.Fill2D({}, {Noise::kMaxDimension + 1, 0}); });
                 Throws<std::length_error>(
                     [&] { noise.Fill2D({}, {Noise::kMaxDimension, Noise::kMaxDimension}); });
                 Throws<std::length_error>([&] { noise.Fill3D({}, {256, 256, 257}); });
                 Throws<std::length_error>(
                     [&] { noise.Fill3D({}, {1, 1, std::numeric_limits<std::size_t>::max()}); });
             });
        Case("grid coordinate validation is atomic",
             [&]
             {
                 std::array<double, 2> field{9, 9};
                 Throws<std::invalid_argument>([&]
                                               { noise.Fill2D(field, {2, 1, 0, 0, 1e9 + 1, 0}); });
                 Throws<std::invalid_argument>(
                     [&] { noise.FillFractal2D(field, {2, 1, 0, 0, 1e9, 0}, {2}); });
                 Throws<std::invalid_argument>(
                     [&] { noise.FillFractal3D(field, {1, 1, 2, 0, 0, 0, 0, 0, -1e9}, {2}); });
                 Check(field == std::array<double, 2>{9, 9},
                       "Failed grid validation changed output.");
             });
        Case("empty grids still validate coordinates and settings",
             [&]
             {
                 Throws<std::invalid_argument>(
                     [&]
                     {
                         noise.Fill2D({},
                                      {0, 0, 0, 0, std::numeric_limits<double>::quiet_NaN(), 1});
                     });
                 Throws<std::invalid_argument>([&] { noise.FillFractal2D({}, {}, {0}); });
                 Throws<std::invalid_argument>([&] { noise.Fill3D({}, {0, 0, 0, 0, 0, 1e9 + 1}); });
             });
        Case("copies and seeds are independent",
             [&]
             {
                 Noise::Generator copy = noise;
                 Near(copy.Sample2D(0.7, -0.2), noise.Sample2D(0.7, -0.2), 0);
                 copy = Noise::Generator(0);
                 Check(copy.Sample2D(0, 0) != noise.Sample2D(0, 0), "Seeds have no effect.");
                 Check(noise.GetSeed() == 42, "Copy assignment affected original.");
             });
        Case("concurrent const sampling",
             [&]
             {
                 auto sum = [&]
                 {
                     double result = 0;
                     for (int i = -500; i < 500; ++i)
                     {
                         result += noise.Sample3D(i * 0.07, i * 0.013, i * -0.017);
                     }
                     return result;
                 };
                 const double expected = sum();
                 std::array<double, 4> actual{};
                 std::vector<std::jthread> threads;
                 for (std::size_t i = 0; i < actual.size(); ++i)
                 {
                     threads.emplace_back([&, i] { actual[i] = sum(); });
                 }
                 threads.clear();
                 for (double value : actual)
                 {
                     Near(value, expected, 0);
                 }
             });
        std::cout << "Contract cases passed: " << cases << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
