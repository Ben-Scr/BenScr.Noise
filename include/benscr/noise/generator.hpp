#ifndef BENSCR_NOISE_GENERATOR_HPP
#define BENSCR_NOISE_GENERATOR_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace BenScr::Noise
{
inline constexpr int kAlgorithmVersion = 1;
inline constexpr double kMaxCoordinate = 1'000'000'000.0;
inline constexpr std::size_t kMaxDimension = 1'048'576;
inline constexpr std::size_t kMaxFieldElements = 16'777'216;
inline constexpr int kMaxOctaves = 32;

/// Normalized fractal value-noise settings. Octaves: [1,32], Frequency: finite and >0,
/// Lacunarity: finite and >=1, Gain: finite and in [0,1]. Every octave is validated.
struct FractalSettings
{
    int Octaves = 5;
    double Frequency = 1.0;
    double Lacunarity = 2.0;
    double Gain = 0.5;
};

/// Row-major samples at (OriginX+x*StepX, OriginY+y*StepY), x changes fastest.
/// Dimensions may be zero. Steps may be zero or negative, but must be finite.
struct Grid2D
{
    std::size_t Width = 0;
    std::size_t Height = 0;
    double OriginX = 0.0;
    double OriginY = 0.0;
    double StepX = 1.0;
    double StepY = 1.0;
};

/// Samples at Origin + index*Step. Index is (z*Height+y)*Width+x.
struct Grid3D
{
    std::size_t Width = 0;
    std::size_t Height = 0;
    std::size_t Depth = 0;
    double OriginX = 0.0;
    double OriginY = 0.0;
    double OriginZ = 0.0;
    double StepX = 1.0;
    double StepY = 1.0;
    double StepZ = 1.0;
};

/// Seeded quintic-interpolated lattice value noise. Results are in [-1,1].
/// No global state or borrowed storage. Concurrent const calls are safe if the object
/// is not assigned concurrently. Valid sampling/filling calls allocate no dynamic storage.
class Generator
{
  public:
    explicit Generator(std::uint64_t seed = 0) noexcept;
    [[nodiscard]] std::uint64_t GetSeed() const noexcept;

    /// Coordinates must be finite and in [-kMaxCoordinate,kMaxCoordinate].
    /// Throws invalid_argument for invalid coordinates.
    [[nodiscard]] double Sample2D(double x, double y) const;
    [[nodiscard]] double Sample3D(double x, double y, double z) const;

    /// Samples all octaves from this seed, scales coordinates by successive frequencies,
    /// and divides the amplitude-weighted sum by total amplitude.
    /// Invalid settings or any out-of-range scaled coordinate throw invalid_argument.
    [[nodiscard]] double SampleFractal2D(double x, double y,
                                         const FractalSettings &settings = {}) const;
    [[nodiscard]] double SampleFractal3D(double x, double y, double z,
                                         const FractalSettings &settings = {}) const;

    /// Destination length must equal the dimension product, bounded by kMaxFieldElements.
    /// Each dimension is <=kMaxDimension. Empty grids require an empty span; their
    /// origin/step/settings still undergo validation. No writes occur on invalid input.
    /// Throws invalid_argument (coordinates/settings/span size) or length_error (dimensions).
    void Fill2D(std::span<double> destination, const Grid2D &grid) const;
    void Fill3D(std::span<double> destination, const Grid3D &grid) const;
    void FillFractal2D(std::span<double> destination, const Grid2D &grid,
                       const FractalSettings &settings = {}) const;
    void FillFractal3D(std::span<double> destination, const Grid3D &grid,
                       const FractalSettings &settings = {}) const;

  private:
    [[nodiscard]] double Lattice(std::int64_t x, std::int64_t y, std::int64_t z) const noexcept;
    [[nodiscard]] double Unchecked2D(double x, double y) const noexcept;
    [[nodiscard]] double Unchecked3D(double x, double y, double z) const noexcept;
    std::uint64_t seed;
};
} // namespace BenScr::Noise
#endif
