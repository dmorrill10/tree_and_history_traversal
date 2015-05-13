#include <vector>

#include <lib/utils.hpp>
#include <lib/policy_generator.hpp>
#include <lib/matrix_game.hpp>

using namespace TreeAndHistoryTraversal;
using namespace PolicyGenerator;
using namespace MatrixGame;

static void runPerturbedCfrWithNoiseAndExploitabilityThreshold(double noise, size_t randomSeed, double exploitability = 1e-4) {
  const std::vector<size_t> numActionsAtEachInfoSet{2};
  std::vector<std::vector<int>> utilsForPlayer1{{2, -2}, {-4, 3}};
  const auto altPolicyGeneratorProfileFactory = [&]() {
    return std::vector<
        PolicyGenerator<size_t, std::pair<size_t, size_t>, Numeric> *>{
      new PerturbedPolicyRegretMatchingTable(
          NUM_SEQUENCES, numActionsAtEachInfoSet,
          NUM_SEQUENCES_BEFORE_EACH_INFO_SET, noise, randomSeed),
              new PerturbedPolicyRegretMatchingTable(
                  NUM_SEQUENCES, numActionsAtEachInfoSet,
                  NUM_SEQUENCES_BEFORE_EACH_INFO_SET, noise, randomSeed)};
  };
  Cfr<size_t, std::pair<size_t, size_t>, Numeric>
              patient(utilsForPlayer1, altPolicyGeneratorProfileFactory());
  size_t t = 1;
  while (true) {
    patient.doIteration();
    if (patient.averageExploitability() < exploitability) {
      printf("%20lg%20zu%20lg\n", noise, t, patient.averageExploitability());
      fflush(NULL);

      if (patient.averageExploitability() < exploitability) {
        return;
      }
    }
    else if (t % 100000 == 0) {
      printf("#%19lg%20zu%20lg\n", noise, t, patient.averageExploitability());
      fflush(NULL);
    }
    ++t;
  }
}

int main(int argc, char** argv) {
  size_t seed = (argc < 2) ? 3839203241 : std::stoul(std::string(argv[1]));
  runPerturbedCfrWithNoiseAndExploitabilityThreshold(0, 0);
  double noise = 0.1;
  for (size_t i = 0; i < 4; ++i) {
    runPerturbedCfrWithNoiseAndExploitabilityThreshold(noise, seed);
    noise *= 5;
    runPerturbedCfrWithNoiseAndExploitabilityThreshold(noise, seed);
    noise *= 2;
  }
}
