/**
* @copyright 2025 - Max Bebök
* @license MIT
*/
#pragma once
#include "main.h"
#include <functional>

// Global variable for debug weapon selection
extern int debugWeaponSelection;
extern bool isForceAllPlayers;

namespace DebugMenu
{
  enum class EntryType : uint32_t {
    INT=0, FLOAT, BOOL, NONE
  };

  struct Entry
  {
    const char* name;
    EntryType type{};
    void* value{nullptr};

    float min{0.0f};
    float max{1.0f};
    float incr{1.0f};
  };

  void reset();
  void addEntry(const Entry& entry, bool *changedFlag = nullptr);

  void draw();
  bool isReturnToMainMenuRequested();
  void reloadSettings();
}