#include "AppChartTable.h"
#include "AppChart.h"
#include "BuffIds.h"
#include "History.h"
#include "Settings.h"

void SetBgColorFake(ImGuiTableBgTarget target, ImU32 color, int column_n = -1) {
	// ToDo: implement
}

static std::vector<ArcdpsExtension::MainTableColumn> tableColumns;

void AppChartTable::InitTableColumns() {
	tableColumns.reserve(tracked_buffs.size() + 3);

	tableColumns.emplace_back(nameColumnId, []() { return ArcdpsExtension::Localization::STranslate(BT_NameColumnHeader); }, []() { return nullptr; }, "0");
	tableColumns.emplace_back(subgroupColumnId, []() { return ArcdpsExtension::Localization::STranslate(BT_SubgroupColumnHeader); }, []() { return nullptr; }, "0");

	auto& iconLoader = ArcdpsExtension::IconLoader::instance();
	ImU32 buffColumnId = 0;
	for (const BoonDef& trackedBuff : tracked_buffs) {

		tableColumns.emplace_back(buffColumnId,
			[&trackedBuff]() {
				return ArcdpsExtension::Localization::STranslate(trackedBuff.name);
			},
			[&iconLoader, &trackedBuff]() {
				return iconLoader.Draw(trackedBuff.iconTextureId);
			},
		"1", trackedBuff.is_relevant);
		++buffColumnId;
	}

	tableColumns.emplace_back(above90ColumnId, []() { return ArcdpsExtension::Localization::STranslate(above90BoonDef->name); },
		[&iconLoader]() {
			return iconLoader.Draw(above90BoonDef->iconTextureId);
		},
	"0");

	ImU32 columnId = 0;
}

AppChartTable::AppChartTable(AppChart* pMainWindow, int windowIndex) : MainTable(tableColumns, pMainWindow, ArcdpsExtension::MainTableFlags_SubWindow), mWindowIndex(windowIndex), mMainWindow(pMainWindow) {
    mTableId = "BoonTable" + std::to_string(mWindowIndex);
}

Alignment& AppChartTable::getAlignment() {
	return settings.getAlignment(mWindowIndex);
}

Alignment& AppChartTable::getHeaderAlignment() {
	// ToDo: implement (different from getAlignment)
	return settings.getAlignment(mWindowIndex);
}

std::string AppChartTable::getTableId() {
	return mTableId;
}

int& AppChartTable::getMaxDisplayed() {
	return settings.getMaxDisplayed(mWindowIndex);
}

const char* AppChartTable::getCategoryName(const std::string& pCat) {
	// ToDo: implement
	return "Cat1";
}

bool& AppChartTable::getShowAlternatingBackground() {
	// ToDo: implement
    static bool showAlternatingBackground = false;
    return showAlternatingBackground;
}

ArcdpsExtension::MainTable<128>::TableSettings& AppChartTable::getTableSettings() {
	// ToDo: implement
	static TableSettings tableSettings;
    return tableSettings;
}

bool& AppChartTable::getHighlightHoveredRows() {
	// ToDo: implement
	static bool highlightHoveredRows = false;
    return highlightHoveredRows;
}

bool& AppChartTable::getShowHeaderAsText() {
	return settings.isShowLabel(mWindowIndex);
}

void AppChartTable::DrawRows(ArcdpsExtension::TableColumnIdx pFirstColumnIndex) {
	int index = mWindowIndex;
	Alignment alignment = settings.getAlignment(index);

	std::lock_guard lockPlayerOrder(playerOrderMtx);
	std::unique_lock<std::mutex> guardHistory = history.lock(std::defer_lock);

	// get current tracker
	ITracker* trackerPtr;
	auto currentHistory = settings.getCurrentHistory(index);
	if (currentHistory == 0) {
		trackerPtr = &liveTracker;
	}
	else {
		guardHistory.lock();
		trackerPtr = &history[currentHistory - 1];
	}

	// Check if the tracker id changed without the user changing anything.
	// This happens when a new log is pushed to the history.
	if (lastTrackerId != trackerPtr->getId() && lastCalculatedHistory == currentHistory)
	{
		// Try finding the new index of the tracker
		std::optional<size_t> newHistoryIndex = history.GetTrackerIndexById(lastTrackerId);
		if (newHistoryIndex.has_value())
		{
			// Found the tracker in history
			currentHistory = newHistoryIndex.value() + 1;
			lastCalculatedHistory = currentHistory;
			trackerPtr = &history[currentHistory - 1];
		}
		else
		{
			// Tracker not found, switch to liveTracker
			currentHistory = 0;
			trackerPtr = &liveTracker;
			guardHistory.unlock();
		}

		settings.setCurrentHistory(index, currentHistory);
	}

	ITracker& tracker = *trackerPtr;
	lastTrackerId = tracker.getId();

	std::lock_guard lock(tracker.players_mtx);

	// if tracker changed, update the playerOrder (new IDs)
	if (lastCalculatedHistory != currentHistory) {
		playerOrder.clear();
		for (const auto& id : tracker.getAllPlayerIds()) {
			playerOrder.emplace_back(id);
		}
		needSort = true;

		lastCalculatedHistory = currentHistory;
	}

	if (ImGuiTableSortSpecs* sorts_specs = GetSortSpecs()) {
		// Sort our data if sort specs have been changed!
		if (sorts_specs->SpecsDirty)
			needSort = true;

		bool expected = true;
		if (needSort.compare_exchange_strong(expected, false)) {
			const bool descend = sorts_specs->Specs->SortDirection == ImGuiSortDirection_Descending;

			if (sorts_specs->Specs->ColumnUserID == nameColumnId) {
				// sort by account name.
				std::ranges::sort(playerOrder, [descend, &tracker](const size_t& playerIdx1, const size_t& playerIdx2) {
					std::string charName1 = tracker.getIEntity(playerIdx1)->getName();
					std::string charName2 = tracker.getIEntity(playerIdx2)->getName();
					std::ranges::transform(charName1, charName1.begin(), [](unsigned char c) { return std::tolower(c); });
					std::ranges::transform(charName2, charName2.begin(), [](unsigned char c) { return std::tolower(c); });

					if (descend) {
						bool res = charName1 < charName2;
						return res;
					}
					return charName1 > charName2;
					});
			}
			else if (sorts_specs->Specs->ColumnUserID == subgroupColumnId) {
				// sort by subgroup
				std::ranges::sort(playerOrder, [descend, &tracker](const size_t& playerIdx1, const size_t& playerIdx2) {
					uint8_t player1Subgroup = tracker.getIPlayer(playerIdx1)->getSubgroup();
					uint8_t player2Subgroup = tracker.getIPlayer(playerIdx2)->getSubgroup();
					if (descend) {
						return player1Subgroup < player2Subgroup;
					}
					return player1Subgroup > player2Subgroup;
					});
			}
			else if (sorts_specs->Specs->ColumnUserID == above90ColumnId) {
				// sort by above 90% hp
				std::ranges::sort(playerOrder, [descend, &tracker](const size_t& playerIdx1, const size_t& playerIdx2) {
					float player1Over90 = tracker.getIEntity(playerIdx1)->getOver90();
					float player2Over90 = tracker.getIEntity(playerIdx2)->getOver90();
					if (descend) {
						return player1Over90 < player2Over90;
					}
					return player1Over90 > player2Over90;
					});
			}
			else {
				// sort by buff
				const ImGuiID buffId = sorts_specs->Specs->ColumnUserID;
				const BoonDef& buff = tracked_buffs[buffId];
				std::ranges::sort(playerOrder, [descend, &tracker, &buff, this](const size_t& playerIdx1, const size_t& playerIdx2) {
					float player1Uptime = tracker.getIEntity(playerIdx1)->getBoonUptime(buff);
					float player2Uptime = tracker.getIEntity(playerIdx2)->getBoonUptime(buff);
					if (descend) {
						return player1Uptime < player2Uptime;
					}
					return player1Uptime > player2Uptime;
					});
			}
			sorts_specs->SpecsDirty = false;
		}
	}

	IPlayer* self_player = tracker.getSelfIPlayer();

	if (settings.isShowSelfOnTop(index)) {
		if (self_player) {
			DrawRow(alignment, self_player->getName(), std::to_string(self_player->getSubgroup()).c_str(), [&](const BoonDef& boonDef) {
				return getEntityDisplayValue(tracker, *self_player, boonDef);
				}, [&self_player]() {
					return self_player->getOver90();
					}, true, self_player, true, settings.getSelfColor());
		}

		NextRow();
		SetBgColorFake(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Separator));
	}

	if (settings.isShowPlayers(index)) {
		bool onlySubgroup = settings.isShowOnlySubgroup(index);
		auto group_filter = [&self_player, onlySubgroup, &tracker](const size_t& playerIdx) {
			if (self_player && onlySubgroup) {
				uint8_t subgroup = self_player->getSubgroup();
				const IPlayer* const player = tracker.getIPlayer(playerIdx);
				return player->getSubgroup() == subgroup;
			}
			return true;
			};
		for (const size_t& playerIdx : playerOrder | std::views::filter(group_filter)) {
			// for (const Player& player : tracker.players | std::views::filter(group_filter)) {
			const IPlayer* const player = tracker.getIPlayer(playerIdx);

			DrawRow(alignment, player->getName(), std::to_string(player->getSubgroup()).c_str(), [&](const BoonDef& boonDef) {
				return getEntityDisplayValue(tracker, *player, boonDef);
				}, [&player]() {
					return player->getOver90();
					}, true, player, player->isSelf(), settings.getSelfColor());
		}
	}

	if (settings.isShowSubgroups(tracker, index)) {
		NextRow();
		SetBgColorFake(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Separator));

		bool onlySubgroup = settings.isShowOnlySubgroup(index);
		auto group_filter = [&self_player, onlySubgroup](const uint8_t& subgroup) {
			if (self_player && onlySubgroup) {
				return subgroup == self_player->getSubgroup();
			}
			return true;
			};

		for (std::set<uint8_t> subgroups = tracker.getSubgroups(); uint8_t subgroup : subgroups | std::views::filter(group_filter)) {
			DrawRow(alignment, ArcdpsExtension::Localization::STranslate(BT_SubgroupNameColumnValue), std::to_string(subgroup).c_str(),
				[&](const BoonDef& boonDef) {
					return tracker.getSubgroupBoonUptime(boonDef, subgroup);
				}, [&tracker, &subgroup]() {
					return tracker.getSubgroupOver90(subgroup);
					});
		}
	}

	if (settings.isShowTotal(index)) {
		DrawRow(alignment, ArcdpsExtension::Localization::STranslate(BT_TotalNameColumnValue), ArcdpsExtension::Localization::STranslate(BT_TotalSubgroupColumnValue).data(),
			[&](const BoonDef& boonDef) {
				return tracker.getAverageBoonUptime(boonDef);
			}, [&tracker]() {
				return tracker.getAverageOver90();
				});
	}
}

void AppChartTable::DrawRow(Alignment alignment, std::string_view charnameText, const char* subgroupText, std::function<float(const BoonDef&)> uptimeFunc,
	std::function<float()> above90Func, bool hasEntity, const IEntity* const entity, bool hasColor, const ImVec4& color) {
	NextRow();

	if (hasColor) {
		ImGui::PushStyleColor(ImGuiCol_Text, color);
	}

	// charname
	NextColumn();
	int maxPlayerLength = settings.getMaxPlayerLength(mWindowIndex);
	std::string shortenedCharname;
	if (maxPlayerLength == 0 || charnameText.length() <= maxPlayerLength) {
		shortenedCharname = charnameText;
	}
	else {
		shortenedCharname = charnameText.substr(0, maxPlayerLength);
	}
	ImGui::TextUnformatted(shortenedCharname.c_str());

	// subgroup
	if (NextColumn()) {
		AlignedTextColumn(subgroupText);
	}

	if (hasColor) {
		ImGui::PopStyleColor();
	}

	// buffs
	for (const BoonDef& trackedBuff : tracked_buffs) {
		if (NextColumn()) {
			float averageBoonUptime = uptimeFunc(trackedBuff);
			if (hasEntity) {
				buffProgressBar(trackedBuff, averageBoonUptime, ImGui::GetColumnWidth(), *entity);
			}
			else {
				buffProgressBar(trackedBuff, averageBoonUptime, ImGui::GetColumnWidth());
			}
		}
	}

	// above90
	if (NextColumn()) {
		const float above90 = above90Func();
		if (hasEntity) {
			buffProgressBar(*above90BoonDef, above90, ImGui::GetColumnWidth(), *entity);
		}
		else {
			buffProgressBar(*above90BoonDef, above90, ImGui::GetColumnWidth());
		}
	}

	EndMaxHeightRow();
	// ToDo: remove
	mMainWindow->endOfRow();
}

void AppChartTable::buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width, ImVec4 color) {
	ProgressBarColoringMode show_colored = settings.getShowColored(mWindowIndex);
	Alignment alignment = settings.getAlignment(mWindowIndex);

	bool hidden_color = false;
	if (color.w == 0.f) hidden_color = true;

	if (settings.isShowBoonAsProgressBar(mWindowIndex)) {
		if (show_colored != ProgressBarColoringMode::Uncolored && !hidden_color) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);

		char label[10];
		if (current_buff.stacking_type == StackingType_intensity) {
			sprintf(label, "%.1f", current_boon_uptime);
			current_boon_uptime /= current_buff.max_stacks;
			// ImGui::ProgressBar(current_boon_uptime, ImVec2(width, ImGui::GetFontSize()), label);
			ImGuiEx::AlignedProgressBar(current_boon_uptime, ImVec2(width, ImGui::GetFontSize()), label, alignment);
		}
		else {
			sprintf(label, "%.0f%%", 100 * current_boon_uptime);
			// ImGui::ProgressBar(current_boon_uptime, ImVec2(width, ImGui::GetFontSize()), label);
			ImGuiEx::AlignedProgressBar(current_boon_uptime, ImVec2(width, ImGui::GetFontSize()), label, alignment);
		}

		if (show_colored != ProgressBarColoringMode::Uncolored && !hidden_color) ImGui::PopStyleColor();
	}
	else {
		if (show_colored != ProgressBarColoringMode::Uncolored && !hidden_color) {
			color.w = 1;
			ImGui::PushStyleColor(ImGuiCol_Text, color);
		}

		if (current_buff.stacking_type == StackingType_intensity) {
			//don't show the % for intensity stacking buffs
			AlignedTextColumn("{:.1f}", current_boon_uptime);
		}
		else {
			AlignedTextColumn("{:.0f}%", 100 * current_boon_uptime);
		}

		if (show_colored != ProgressBarColoringMode::Uncolored && !hidden_color) ImGui::PopStyleColor();
	}
}

void AppChartTable::buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width) {
	switch (settings.getShowColored(mWindowIndex)) {
	case ProgressBarColoringMode::ByPercentage: {
		float percentage = 0;
		if (current_buff.stacking_type == StackingType_intensity) {
			percentage = current_boon_uptime / current_buff.max_stacks;
		}
		else {
			percentage = current_boon_uptime;
		}
		ImVec4 _0Color = settings.get0Color();
		ImVec4 _100Color = settings.get100Color();
		_0Color = _0Color * (1 - percentage);
		_100Color = _100Color * percentage;
		ImVec4 color = _100Color + _0Color;
		buffProgressBar(current_buff, current_boon_uptime, width, color);
		break;
	}
	default: buffProgressBar(current_buff, current_boon_uptime, width, ImVec4(0, 0, 0, 0));
	}
}

void AppChartTable::buffProgressBar(const BoonDef& current_buff, float current_boon_uptime, float width, const IEntity& entity) {
	switch (settings.getShowColored(mWindowIndex)) {
	case ProgressBarColoringMode::ByProfession:
		buffProgressBar(current_buff, current_boon_uptime, width, entity.getColor());
		break;
	case ProgressBarColoringMode::ByPercentage: {
		float percentage = 0;
		if (current_buff.stacking_type == StackingType_intensity) {
			percentage = current_boon_uptime / current_buff.max_stacks;
		}
		else {
			percentage = current_boon_uptime;
		}
		ImVec4 _0Color = settings.get0Color();
		ImVec4 _100Color = settings.get100Color();
		_0Color = _0Color * (1 - percentage);
		_100Color = _100Color * percentage;
		ImVec4 color = _100Color + _0Color;
		buffProgressBar(current_buff, current_boon_uptime, width, color);
		break;
	}
	default:
		buffProgressBar(current_buff, current_boon_uptime, width, ImVec4(0, 0, 0, 0));
	}
}

void AppChartTable::Sort(const ImGuiTableColumnSortSpecs* mColumnSortSpecs) {
	// ToDo: implement
}

uint8_t AppChartTable::getCurrentHistory() const {
	return settings.getCurrentHistory(mWindowIndex);
}

void AppChartTable::removePlayer(size_t playerId) {
	std::lock_guard lock(playerOrderMtx);

	if (getCurrentHistory() != 0) {
		// We are showing a historical tracker, do not change the player order.
		return;
	}

	auto s = playerOrder.size();
	std::erase_if(playerOrder, [&playerId](const size_t& idx) {
		return idx == playerId;
		});
}

void AppChartTable::addPlayer(size_t playerId) {
	std::lock_guard lock(playerOrderMtx);

	if (getCurrentHistory() != 0) {
		// We are showing a historical tracker, do not change the player order.
		return;
	}

	playerOrder.emplace_back(playerId);
}

float AppChartTable::getEntityDisplayValue(const ITracker& tracker, const IEntity& entity, const BoonDef& boon) {
	return entity.getBoonUptime(boon);
}

void AppChartTable::sortNeeded() {
	needSort = true;
}
