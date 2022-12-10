#include <render/sampler.h>

thread_local std::mt19937_64 Sampler::prng{};
std::uniform_real_distribution<float> Sampler::randFloat{0.0f, 1.0f};
