#include <benscr/noise/generator.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace BenScr::Noise
{
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53,
              "BenScr.Noise requires IEEE 754 binary64 double.");
namespace Detail
{
constexpr std::uint64_t kGolden = 0x9E3779B97F4A7C15ULL;
constexpr std::uint64_t kMultiplier1 = 0xBF58476D1CE4E5B9ULL;
constexpr std::uint64_t kMultiplier2 = 0x94D049BB133111EBULL;

std::uint64_t Mix(std::uint64_t value) noexcept
{
    value = (value ^ (value >> 30)) * kMultiplier1;
    value = (value ^ (value >> 27)) * kMultiplier2;
    return value ^ (value >> 31);
}

double Fade(double value) noexcept
{
    return std::clamp(value * value * value * (value * (value * 6.0 - 15.0) + 10.0), 0.0, 1.0);
}

void CheckCoordinate(double value)
{
    if (!std::isfinite(value) || value < -kMaxCoordinate || value > kMaxCoordinate)
    {
        throw std::invalid_argument("Noise coordinates must be finite and within +/-1e9.");
    }
}

double CheckAxis(double origin, double step, std::size_t size)
{
    CheckCoordinate(origin);
    if (!std::isfinite(step))
    {
        throw std::invalid_argument("Grid steps must be finite.");
    }
    const double last = size == 0 ? origin : origin + static_cast<double>(size - 1) * step;
    CheckCoordinate(last);
    return last;
}

std::size_t CheckSize(std::span<double> destination, std::size_t width, std::size_t height,
                      std::size_t depth)
{
    if (width > kMaxDimension || height > kMaxDimension || depth > kMaxDimension)
    {
        throw std::length_error("A grid dimension exceeds the supported limit.");
    }
    std::size_t total = 0;
    if (width != 0 && height != 0 && depth != 0)
    {
        if (width > kMaxFieldElements / height)
        {
            throw std::length_error("The grid exceeds the element limit.");
        }
        total = width * height;
        if (total > kMaxFieldElements / depth)
        {
            throw std::length_error("The grid exceeds the element limit.");
        }
        total *= depth;
    }
    if (destination.size() != total)
    {
        throw std::invalid_argument("Destination length must equal the grid element count.");
    }
    return total;
}

struct Octaves
{
    std::array<double, kMaxOctaves> Frequencies{};
    std::array<double, kMaxOctaves> Amplitudes{};
    int Count;
    double TotalAmplitude = 0.0;
};

Octaves Prepare(const FractalSettings &settings)
{
    if (settings.Octaves < 1 || settings.Octaves > kMaxOctaves ||
        !std::isfinite(settings.Frequency) || settings.Frequency <= 0.0 ||
        !std::isfinite(settings.Lacunarity) || settings.Lacunarity < 1.0 ||
        !std::isfinite(settings.Gain) || settings.Gain < 0.0 || settings.Gain > 1.0)
    {
        throw std::invalid_argument("Invalid fractal settings.");
    }
    Octaves result{{}, {}, settings.Octaves};
    double frequency = settings.Frequency;
    double amplitude = 1.0;
    for (int octave = 0; octave < settings.Octaves; ++octave)
    {
        if (!std::isfinite(frequency))
        {
            throw std::invalid_argument("An octave frequency is not finite.");
        }
        result.Frequencies[static_cast<std::size_t>(octave)] = frequency;
        result.Amplitudes[static_cast<std::size_t>(octave)] = amplitude;
        result.TotalAmplitude += amplitude;
        if (octave + 1 < settings.Octaves)
        {
            frequency *= settings.Lacunarity;
            amplitude *= settings.Gain;
        }
    }
    return result;
}

void CheckScaled(double first, double last, const Octaves &octaves)
{
    CheckCoordinate(first);
    CheckCoordinate(last);
    for (int octave = 0; octave < octaves.Count; ++octave)
    {
        const double frequency = octaves.Frequencies[static_cast<std::size_t>(octave)];
        CheckCoordinate(first * frequency);
        CheckCoordinate(last * frequency);
    }
}

template <typename Sample> double Fractal(const Octaves &octaves, Sample sample) noexcept
{
    double sum = 0.0;
    for (int octave = 0; octave < octaves.Count; ++octave)
    {
        const auto index = static_cast<std::size_t>(octave);
        sum += octaves.Amplitudes[index] * sample(octaves.Frequencies[index]);
    }
    return std::clamp(sum / octaves.TotalAmplitude, -1.0, 1.0);
}
} // namespace Detail

Generator::Generator(std::uint64_t seed) noexcept : seed(seed)
{
}

std::uint64_t Generator::GetSeed() const noexcept
{
    return seed;
}

double Generator::Lattice(std::int64_t x, std::int64_t y, std::int64_t z) const noexcept
{
    const std::uint64_t hash =
        Detail::Mix((seed + Detail::kGolden) ^ (static_cast<std::uint64_t>(x) * Detail::kGolden) ^
                    (static_cast<std::uint64_t>(y) * Detail::kMultiplier1) ^
                    (static_cast<std::uint64_t>(z) * Detail::kMultiplier2));
    return static_cast<double>(hash >> 11) * 0x1.0p-52 - 1.0;
}

double Generator::Unchecked2D(double x, double y) const noexcept
{
    const double floorX = std::floor(x);
    const double floorY = std::floor(y);
    const auto ix = static_cast<std::int64_t>(floorX);
    const auto iy = static_cast<std::int64_t>(floorY);
    const double tx = Detail::Fade(x - floorX);
    const double ty = Detail::Fade(y - floorY);
    return std::lerp(std::lerp(Lattice(ix, iy, 0), Lattice(ix + 1, iy, 0), tx),
                     std::lerp(Lattice(ix, iy + 1, 0), Lattice(ix + 1, iy + 1, 0), tx), ty);
}

double Generator::Unchecked3D(double x, double y, double z) const noexcept
{
    const double floorX = std::floor(x);
    const double floorY = std::floor(y);
    const double floorZ = std::floor(z);
    const auto ix = static_cast<std::int64_t>(floorX);
    const auto iy = static_cast<std::int64_t>(floorY);
    const auto iz = static_cast<std::int64_t>(floorZ);
    const double tx = Detail::Fade(x - floorX);
    const double ty = Detail::Fade(y - floorY);
    const double tz = Detail::Fade(z - floorZ);
    const double lower =
        std::lerp(std::lerp(Lattice(ix, iy, iz), Lattice(ix + 1, iy, iz), tx),
                  std::lerp(Lattice(ix, iy + 1, iz), Lattice(ix + 1, iy + 1, iz), tx), ty);
    const double upper =
        std::lerp(std::lerp(Lattice(ix, iy, iz + 1), Lattice(ix + 1, iy, iz + 1), tx),
                  std::lerp(Lattice(ix, iy + 1, iz + 1), Lattice(ix + 1, iy + 1, iz + 1), tx), ty);
    return std::lerp(lower, upper, tz);
}

double Generator::Sample2D(double x, double y) const
{
    Detail::CheckCoordinate(x);
    Detail::CheckCoordinate(y);
    return Unchecked2D(x, y);
}

double Generator::Sample3D(double x, double y, double z) const
{
    Detail::CheckCoordinate(x);
    Detail::CheckCoordinate(y);
    Detail::CheckCoordinate(z);
    return Unchecked3D(x, y, z);
}

double Generator::SampleFractal2D(double x, double y, const FractalSettings &settings) const
{
    const auto octaves = Detail::Prepare(settings);
    Detail::CheckScaled(x, x, octaves);
    Detail::CheckScaled(y, y, octaves);
    return Detail::Fractal(octaves, [this, x, y](double frequency)
                           { return Unchecked2D(x * frequency, y * frequency); });
}

double Generator::SampleFractal3D(double x, double y, double z,
                                  const FractalSettings &settings) const
{
    const auto octaves = Detail::Prepare(settings);
    Detail::CheckScaled(x, x, octaves);
    Detail::CheckScaled(y, y, octaves);
    Detail::CheckScaled(z, z, octaves);
    return Detail::Fractal(octaves, [this, x, y, z](double frequency)
                           { return Unchecked3D(x * frequency, y * frequency, z * frequency); });
}

void Generator::Fill2D(std::span<double> destination, const Grid2D &grid) const
{
    Detail::CheckSize(destination, grid.Width, grid.Height, 1);
    Detail::CheckAxis(grid.OriginX, grid.StepX, grid.Width);
    Detail::CheckAxis(grid.OriginY, grid.StepY, grid.Height);
    for (std::size_t y = 0; y < grid.Height && grid.Width != 0; ++y)
    {
        for (std::size_t x = 0; x < grid.Width; ++x)
        {
            destination[y * grid.Width + x] =
                Unchecked2D(grid.OriginX + static_cast<double>(x) * grid.StepX,
                            grid.OriginY + static_cast<double>(y) * grid.StepY);
        }
    }
}

void Generator::Fill3D(std::span<double> destination, const Grid3D &grid) const
{
    const auto count = Detail::CheckSize(destination, grid.Width, grid.Height, grid.Depth);
    Detail::CheckAxis(grid.OriginX, grid.StepX, grid.Width);
    Detail::CheckAxis(grid.OriginY, grid.StepY, grid.Height);
    Detail::CheckAxis(grid.OriginZ, grid.StepZ, grid.Depth);
    if (count == 0)
    {
        return;
    }
    for (std::size_t z = 0; z < grid.Depth; ++z)
    {
        for (std::size_t y = 0; y < grid.Height; ++y)
        {
            for (std::size_t x = 0; x < grid.Width; ++x)
            {
                destination[(z * grid.Height + y) * grid.Width + x] =
                    Unchecked3D(grid.OriginX + static_cast<double>(x) * grid.StepX,
                                grid.OriginY + static_cast<double>(y) * grid.StepY,
                                grid.OriginZ + static_cast<double>(z) * grid.StepZ);
            }
        }
    }
}

void Generator::FillFractal2D(std::span<double> destination, const Grid2D &grid,
                              const FractalSettings &settings) const
{
    const auto octaves = Detail::Prepare(settings);
    Detail::CheckSize(destination, grid.Width, grid.Height, 1);
    const double lastX = Detail::CheckAxis(grid.OriginX, grid.StepX, grid.Width);
    const double lastY = Detail::CheckAxis(grid.OriginY, grid.StepY, grid.Height);
    Detail::CheckScaled(grid.OriginX, lastX, octaves);
    Detail::CheckScaled(grid.OriginY, lastY, octaves);
    for (std::size_t y = 0; y < grid.Height && grid.Width != 0; ++y)
    {
        for (std::size_t x = 0; x < grid.Width; ++x)
        {
            const double px = grid.OriginX + static_cast<double>(x) * grid.StepX;
            const double py = grid.OriginY + static_cast<double>(y) * grid.StepY;
            destination[y * grid.Width + x] = Detail::Fractal(
                octaves, [this, px, py](double f) { return Unchecked2D(px * f, py * f); });
        }
    }
}

void Generator::FillFractal3D(std::span<double> destination, const Grid3D &grid,
                              const FractalSettings &settings) const
{
    const auto octaves = Detail::Prepare(settings);
    const auto count = Detail::CheckSize(destination, grid.Width, grid.Height, grid.Depth);
    const double lastX = Detail::CheckAxis(grid.OriginX, grid.StepX, grid.Width);
    const double lastY = Detail::CheckAxis(grid.OriginY, grid.StepY, grid.Height);
    const double lastZ = Detail::CheckAxis(grid.OriginZ, grid.StepZ, grid.Depth);
    Detail::CheckScaled(grid.OriginX, lastX, octaves);
    Detail::CheckScaled(grid.OriginY, lastY, octaves);
    Detail::CheckScaled(grid.OriginZ, lastZ, octaves);
    if (count == 0)
    {
        return;
    }
    for (std::size_t z = 0; z < grid.Depth; ++z)
    {
        for (std::size_t y = 0; y < grid.Height; ++y)
        {
            for (std::size_t x = 0; x < grid.Width; ++x)
            {
                const double px = grid.OriginX + static_cast<double>(x) * grid.StepX;
                const double py = grid.OriginY + static_cast<double>(y) * grid.StepY;
                const double pz = grid.OriginZ + static_cast<double>(z) * grid.StepZ;
                destination[(z * grid.Height + y) * grid.Width + x] =
                    Detail::Fractal(octaves, [this, px, py, pz](double f)
                                    { return Unchecked3D(px * f, py * f, pz * f); });
            }
        }
    }
}
} // namespace BenScr::Noise
