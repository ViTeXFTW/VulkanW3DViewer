

#include <filesystem>
#include <optional>
namespace w3d {
class ApplicationPaths {
public:
  static constexpr const char *kApplicationName = "VulkanW3DViewer";

  static std::optional<std::filesystem::path> appDataDir();
  static std::optional<std::filesystem::path> imguiIniPath();
  static std::optional<std::filesystem::path> settingsFilePath();
  static bool ensureAppDataDirExists();
};
} // namespace w3d
