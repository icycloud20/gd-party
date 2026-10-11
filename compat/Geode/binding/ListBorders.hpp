#pragma once

// Geode 5 exposes ListBorders from Geode/ui/General.hpp rather than a
// generated binding header. Keep this compatibility include so UI code can
// use the class under the same include path across SDK layouts.
#include <Geode/ui/General.hpp>
