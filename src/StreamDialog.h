#pragma once
#include <windows.h>
#include <string>
#include <vector>

// Show a modal dialog listing the alternate data streams of each given file.
void ShowStreamDialog(HWND parent, const std::vector<std::wstring>& files);
