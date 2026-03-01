#pragma once
#include "ITracker.h"
#include <ArcdpsExtension/Windows/MainTable.h>
#include <ArcdpsExtension/Windows/MainWindow.h>
#include <imgui/imgui.h>
#include <atomic>
#include <mutex>
#include <string>

class AppChart;

class AppChartTable final : public ArcdpsExtension::MainTable<128>
{
public:
	explicit AppChartTable(AppChart* pMainWindow, int windowIndex);
	static void InitTableColumns();

	void removePlayer(size_t playerId);
	void addPlayer(size_t playerId);
	void sortNeeded();

protected:
	Alignment& getAlignment() override;
	Alignment& getHeaderAlignment() override;
	std::string getTableId() override;
	int& getMaxDisplayed() override;
	const char* getCategoryName(const std::string& pCat) override;
	bool& getShowAlternatingBackground() override;
	TableSettings& getTableSettings() override;
	bool& getHighlightHoveredRows() override;
	bool& getShowHeaderAsText() override;

	void DrawRows(ArcdpsExtension::TableColumnIdx pFirstColumnIndex) override;
	void Sort(const ImGuiTableColumnSortSpecs* mColumnSortSpecs) override;
private:
	uint8_t getCurrentHistory() const;
	float getEntityDisplayValue(const ITracker& tracker, const IEntity& entity, const BoonDef& boon);
	void DrawRow(Alignment alignment, std::string_view charnameText, const char* subgroupText, std::function<float(const BoonDef&)> uptimeFunc, std::function<float()>
		above90Func, bool hasEntity = false, const IEntity* const entity = nullptr, bool hasColor = false, const ImVec4& color = ImVec4(0, 0, 0, 0));
	void buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width, ImVec4 color);
	void buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width);
	void buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width, const IEntity& entity);

	AppChart* mMainWindow;
	std::string mTableId;
	int mWindowIndex;

	std::atomic_bool needSort;
	std::vector<size_t> playerOrder;
	std::mutex playerOrderMtx;
	uint8_t lastCalculatedHistory = 0;
	uint64_t lastTrackerId = 0;

	static const ImU32 nameColumnId = 128 - 1;
	static const ImU32 subgroupColumnId = 128 - 2;
	static const ImU32 above90ColumnId = 128 - 3;
};

