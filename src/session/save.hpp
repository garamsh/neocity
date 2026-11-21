#pragma once

#include <survival/save.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace neocity::session {

/// One overwrite-only day slot. Tests supply a temporary path; the host
/// supplies the engine's resolved user path. File failures never stop play.
class SaveSlot {
public:
  explicit SaveSlot(std::filesystem::path file) : path(std::move(file)) {}

  /// Reads a bounded file and returns only a parsed checkpoint.
  [[nodiscard]] std::optional<survival::DaySave>
  load(const survival::Content &content) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      return std::nullopt;
    }
    std::string bytes(131073, '\0');
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (input.bad()) {
      return std::nullopt;
    }
    bytes.resize(static_cast<std::size_t>(input.gcount()));
    return survival::DaySave::parse(bytes, content);
  }

  /// Deletes a terminal run's checkpoint, or overwrites at a fresh day.
  /// False reports an I/O failure to the host's single error handler.
  [[nodiscard]] bool update(const survival::Simulation &simulation,
                            const survival::Workshop &workshop,
                            const survival::Exploration &exploration,
                            survival::ControlMap players) const {
    if (survival::run_over(simulation.snapshot().phase)) {
      std::error_code error;
      std::filesystem::remove(path, error);
      return !error;
    }
    const auto save =
        survival::DaySave::capture(simulation, workshop, exploration, players);
    if (!save) {
      std::abort();
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
      return false;
    }
    output << save->encode();
    output.close();
    return !output.fail();
  }

private:
  std::filesystem::path path;
};

} // namespace neocity::session
