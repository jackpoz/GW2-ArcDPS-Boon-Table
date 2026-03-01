#pragma once

#include "AppChartTable.h"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ITracker.h"
#include "BuffIds.h"
#include "Settings.h"
#include "BigTable/BigTable.h"
#include <ArcdpsExtension/Windows/MainWindow.h>

class AppChart final : public ArcdpsExtension::MainWindow
{
	friend class AppChartsContainer;
	friend class AppChartTable;
public:
	void sortNeeded();
	
	AppChart(int new_index);
	bool& GetOpenVar() override;

	void removePlayer(size_t playerId);
	void addPlayer(size_t playerId);

private:
	void DrawContextMenu() override;
	void DrawContent() override;
	std::string_view getTitleDefault() override;
	std::optional<std::string>& getTitle() override;
	std::string_view getWindowID() override;
	bool& getShowTitleBar() override;
	bool& getShowBackground() override;
	std::optional<ImVec2>& getPadding() override;
	SizingPolicy& getSizingPolicy() override;
	bool getMaxHeightActive() override;
	std::optional<std::string>& getAppearAsInOption() override;
	std::string_view getAppearAsInOptionDefault() override;
	void DrawStyleSettingsSubMenu() override;
	bool& GetShowScrollbar() override;

	ImGuiEx::BigTable::ImGuiTable* imGuiTable = nullptr;
	std::unique_ptr <AppChartTable> mTable;
	int index = 0;

	uint8_t rowCount = 0;
	float maxHeight = 0;
	float minHeight = 0;
	float titleBarHeight = 0;
	float innerTableCursorPos = 0;
	uint8_t nthTick = 0;

	std::string windowID;
	bool mShowScrollbar = false;
	SizingPolicy mSizingPolicy = SizingPolicy::SizeToContent;
	int mCurrentRow = 0;
	std::optional<std::string> mAppearAsInOptionOpt;
	const std::string mAppearAsInOptionDefault = "Demo Window";

	void endOfRow();
};

class AppChartsContainer {
public:
	void removePlayer(uintptr_t playerId);
	void addPlayer(uintptr_t playerId);
	void sortNeeded();
	void drawAll(ImGuiWindowFlags flags);
	
private:
	AppChart charts[MaxTableWindowAmount] {
		0,1,2,3,4
	};
};

extern AppChartsContainer charts;
