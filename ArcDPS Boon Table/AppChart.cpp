#include "AppChart.h"

#include <algorithm>
#include <mutex>
#include <ranges>

#include "Entity.h"
#include "History.h"
#include "Lang.h"
#include "Settings.h"
#include "SettingsUI.h"
#include "Tracker.h"
#include <ArcdpsExtension/Widgets.h>
#include <ArcdpsExtension/ImGui_Math.h>
#include <ArcdpsExtension/IconLoader.h>

AppChartsContainer charts;

namespace Table = ImGuiEx::BigTable;
using ArcdpsExtension::Localization;

AppChart::AppChart(int new_index) : ArcdpsExtension::MainWindow(), index(new_index) {
	windowID = "Boon Table";
	if (index > 0)
		windowID += std::to_string(index);

	RegisterPreDrawHook([this](ImGuiWindowFlags& flags) {
		flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		if (settings.getPosition(index) != Position::Manual) {
			flags |= ImGuiWindowFlags_NoMove;
		}
	});

	mTable = std::make_unique<AppChartTable>(this, index);
}

bool& AppChart::GetOpenVar() {
	return settings.isShowChart(index);
}

std::string_view AppChart::getTitleDefault() {
    return Localization::STranslate(BT_WindowHeader);
}

std::optional<std::string>& AppChart::getTitle() {
	return settings.getTitleBar(index);
}

std::string_view AppChart::getWindowID() {
	return windowID;
}

bool& AppChart::getShowTitleBar() {
    return settings.isHideHeader(index);
}

bool& AppChart::getShowBackground() {
	return settings.isShowBackground(index);
}

std::optional<ImVec2>& AppChart::getPadding() {
	return settings.getWindowPadding(index);
}

SizingPolicy& AppChart::getSizingPolicy() {
	return mSizingPolicy;
}

bool AppChart::getMaxHeightActive() {
	return settings.getMaxDisplayed(index) > 0 && rowCount >= settings.getMaxDisplayed(index);
}

std::optional<std::string>& AppChart::getAppearAsInOption() {
    return mAppearAsInOptionOpt;
}

std::string_view AppChart::getAppearAsInOptionDefault() {
    return mAppearAsInOptionDefault;
}

bool& AppChart::GetShowScrollbar() {
    return mShowScrollbar;
}

void AppChart::DrawStyleSettingsSubMenu() {
	MainWindow::DrawStyleSettingsSubMenu();
}

void AppChart::DrawContextMenu() {
	settingsUi.Draw(imGuiTable, index, ImGui::GetCurrentWindow());
	mTable->DrawColumnSetupMenu();
}

void AppChart::DrawContent() {
	PRINT_LINE()

	if (getMaxHeightActive()) {
		// add the innertable position to the maxHeight
		maxHeight += innerTableCursorPos;
#if _DEBUG
		arc_log(std::format("innerTableCursorPos: {}", innerTableCursorPos).c_str());
#endif
		
		float overriddenHeight = titleBarHeight + ImGui::GetStyle().WindowPadding.y + 5.f;
		overriddenHeight = std::max(overriddenHeight, 50.f);
		minHeight = std::max(minHeight, overriddenHeight);
		maxHeight = std::max(maxHeight, overriddenHeight);
		SetMaxHeightCursorPos(maxHeight - ImGui::GetStyle().WindowPadding.y);
	} else {
		minHeight = titleBarHeight + ImGui::GetStyle().WindowPadding.y + 5.f;
		maxHeight = FLT_MAX;
	}

#if _DEBUG
	arc_log("--------------------");
	arc_log(std::format("maxHeight: {}", maxHeight).c_str());
#endif
	
	rowCount = 0;
	maxHeight = 0;
	minHeight = 0;
	innerTableCursorPos = 0;

	ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();

#if _DEBUG
	arc_log(std::format("actualHeight: {}", ImGui::GetWindowHeight()).c_str());
#endif

	// add window titleBar to defined height
	titleBarHeight = currentWindow->TitleBarHeight();
	minHeight += titleBarHeight;
	maxHeight += titleBarHeight;
#if _DEBUG
	arc_log(std::format("titleBarHeight: {}", titleBarHeight).c_str());
#endif

	// add scrollbar to defined height
	if (imGuiTable && imGuiTable->InnerWindow->ScrollbarX) {
		const ImRect scrollbarRect = ImGui::GetWindowScrollbarRect(imGuiTable->InnerWindow, ImGuiAxis_X);
		float height = scrollbarRect.GetHeight();
		minHeight += height;
		maxHeight += height;

#if _DEBUG
		arc_log(std::format("scrollbarHeight: {}", height).c_str());
#endif
	}

	// add window paddings to defined height
	float paddingHeight = ImGui::GetStyle().WindowPadding.y;
	minHeight += paddingHeight*2;
	maxHeight += paddingHeight*2;

#if _DEBUG
	arc_log(std::format("paddingHeight: {}", paddingHeight).c_str());
#endif

	mTable->Draw();
	/*
	// columns: charname | subgroup | tracked_buffs
	const int columnCount = tracked_buffs.size() + 3;
	// last 3 possible elements are hardcoded ones
	const unsigned int nameColumnId = 128 - 1;
	const unsigned int subgroupColumnId = 128 - 2;
	const unsigned int above90ColumnId = 128 - 3;

	int tableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoBordersInBody;

	auto sizingPolicy = getSizingPolicy();
	switch (sizingPolicy) {
		case SizingPolicy::ManualWindowSize:
			tableFlags |= ImGuiTableFlags_ScrollX;
			[[fallthrough]];
		case SizingPolicy::SizeToContent:
			tableFlags |= ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
			break;
		case SizingPolicy::SizeContentToWindow:
			tableFlags |= ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY;
			break;
	}

	if (settings.isAlternatingRowBg(index)) {
		tableFlags |= ImGuiTableFlags_RowBg;
	}

	if (settings.isTablePaddingX(index)) {
		tableFlags |= ImGuiTableFlags_PadOuterX;
	}

	ImGuiWindowFlags subWindowFlags = 0;
	if (!settings.isScrollbar(index)) {
		subWindowFlags |= ImGuiWindowFlags_NoScrollbar;
	}

	std::string tableId = "BoonTable";
	tableId.append(std::to_string(index));
	if (Table::BeginTable(tableId.c_str(), columnCount, tableFlags, subWindowFlags)) {
		imGuiTable = Table::CurrentTable;

		// lock header so it is not scrolled out of vision
		Table::TableSetupScrollFreeze(0, 1);

		Alignment alignment = settings.getAlignment(index);
		bool showLabel = settings.isShowLabel(index);

		std::string_view charName = Localization::STranslate(BT_NameColumnHeader);
		std::string_view subgroupName = Localization::STranslate(BT_SubgroupColumnHeader);

		ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed;
		Table::TableSetupColumn(charName.data(), columnFlags, 0, nameColumnId);
		Table::TableSetupColumn(subgroupName.data(), columnFlags, 0, subgroupColumnId);

		float init_width = 80.f;
		if (sizingPolicy == SizingPolicy::SizeToContent || sizingPolicy == SizingPolicy::ManualWindowSize) {
			init_width = settings.getBoonColumnWidth(index);
		}

		ImU32 i = 0;
		for (const BoonDef& trackedBuff : tracked_buffs) {
			int bufFlags = ImGuiTableColumnFlags_NoResize;
			if (!trackedBuff.is_relevant) {
				bufFlags |= ImGuiTableColumnFlags_DefaultHide;
			}
			if (i == 0) {
				bufFlags |= ImGuiTableColumnFlags_DefaultSort;
			}

			Table::TableSetupColumn(Localization::STranslate(trackedBuff.name).data(), bufFlags, init_width, i);

			++i;
		}

		Table::TableSetupColumn(Localization::STranslate(above90BoonDef->name).data(), ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_DefaultHide, init_width, above90ColumnId);

		Table::TableNextRow(ImGuiTableRowFlags_Headers);

		// accountname header
		if (Table::TableNextColumn())
			Table::TableHeader(charName.data(), true, nullptr);

		// subgroup header
		if (Table::TableNextColumn())
			Table::TableHeader(subgroupName.data(), true, nullptr);

		auto& iconLoader = ArcdpsExtension::IconLoader::instance();
		// buff headers
		for (const BoonDef& trackedBuff : tracked_buffs) {
			if (Table::TableNextColumn()) {
				Table::TableHeader(Localization::STranslate(trackedBuff.name).data(), showLabel, iconLoader.Draw(trackedBuff.iconTextureId), alignment);
			}
		}

		// above90 header
		if (Table::TableNextColumn()) {
			Table::TableHeader(Localization::STranslate(above90BoonDef->name).data(), showLabel, iconLoader.Draw(above90BoonDef->iconTextureId), alignment);
		}

		std::lock_guard lockPlayerOrder(playerOrderMtx);
		std::unique_lock<std::mutex> guardHistory = history.lock(std::defer_lock);

		// get current tracker
		ITracker* trackerPtr;
		auto currentHistory = settings.getCurrentHistory(index);
		if (currentHistory == 0) {
			trackerPtr = &liveTracker;
		} else {
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

		if (ImGuiTableSortSpecs* sorts_specs = Table::TableGetSortSpecs()) {
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
				} else if (sorts_specs->Specs->ColumnUserID == subgroupColumnId) {
					// sort by subgroup
					std::ranges::sort(playerOrder, [descend, &tracker](const size_t& playerIdx1, const size_t& playerIdx2) {
						uint8_t player1Subgroup = tracker.getIPlayer(playerIdx1)->getSubgroup();
						uint8_t player2Subgroup = tracker.getIPlayer(playerIdx2)->getSubgroup();
						if (descend) {
							return player1Subgroup < player2Subgroup;
						}
						return player1Subgroup > player2Subgroup;
					});
				} else if (sorts_specs->Specs->ColumnUserID == above90ColumnId) {
					// sort by above 90% hp
					std::ranges::sort(playerOrder, [descend, &tracker](const size_t& playerIdx1, const size_t& playerIdx2) {
						float player1Over90 = tracker.getIEntity(playerIdx1)->getOver90();
						float player2Over90 = tracker.getIEntity(playerIdx2)->getOver90();
						if (descend) {
							return player1Over90 < player2Over90;
						}
						return player1Over90 > player2Over90;
					});
				} else {
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

			Table::TableNextRow();
			Table::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Separator));
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
			Table::TableNextRow();
			Table::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Separator));

			bool onlySubgroup = settings.isShowOnlySubgroup(index);
			auto group_filter = [&self_player, onlySubgroup](const uint8_t& subgroup) {
				if (self_player && onlySubgroup) {
					return subgroup == self_player->getSubgroup();
				}
				return true;
			};

			for (std::set<uint8_t> subgroups = tracker.getSubgroups(); uint8_t subgroup : subgroups | std::views::filter(group_filter)) {
				DrawRow(alignment, Localization::STranslate(BT_SubgroupNameColumnValue), std::to_string(subgroup).c_str(),
						[&](const BoonDef& boonDef) {
							return tracker.getSubgroupBoonUptime(boonDef, subgroup);
						}, [&tracker, &subgroup]() {
							return tracker.getSubgroupOver90(subgroup);
						});
			}
		}

		if (settings.isShowTotal(index)) {
			DrawRow(alignment, Localization::STranslate(BT_TotalNameColumnValue), Localization::STranslate(BT_TotalSubgroupColumnValue).data(),
					[&](const BoonDef& boonDef) {
						return tracker.getAverageBoonUptime(boonDef);
					}, [&tracker]() {
						return tracker.getAverageOver90();
					});
		}

		Table::EndTable();
	}
	*/

	if (nthTick >= 10) {
		nthTick = 0;
		ImGuiEx::WindowReposition(nullptr, settings.getPosition(index), settings.getCornerVector(index),
								  settings.getCornerPosition(index), settings.getFromWindowID(index),
								  settings.getAnchorPanelCornerPosition(index), settings.getSelfPanelCornerPosition(index));
	} else {
		++nthTick;
	}
}

/*
void AppChart::DrawRow(Alignment alignment, std::string_view charnameText, const char* subgroupText, std::function<float(const BoonDef&)> uptimeFunc,
					   std::function<float()> above90Func, bool hasEntity, const IEntity* const entity, bool hasColor, const ImVec4& color) {
	Table::TableNextRow();

	if (hasColor) {
		ImGui::PushStyleColor(ImGuiCol_Text, color);
	}

	// charname
	Table::TableNextColumn();
	int maxPlayerLength = settings.getMaxPlayerLength(index);
	std::string shortenedCharname;
	if (maxPlayerLength == 0 || charnameText.length() <= maxPlayerLength) {
		shortenedCharname = charnameText;
	} else {
		shortenedCharname = charnameText.substr(0, maxPlayerLength);
	}
	ImGui::TextUnformatted(shortenedCharname.c_str());

	// subgroup
	if (Table::TableNextColumn()) {
		Table::AlignedTextColumn(alignment, subgroupText);
	}

	if (hasColor) {
		ImGui::PopStyleColor();
	}

	// buffs
	for (const BoonDef& trackedBuff : tracked_buffs) {
		if (Table::TableNextColumn()) {
			float averageBoonUptime = uptimeFunc(trackedBuff);
			if (hasEntity) {
				buffProgressBar(trackedBuff, averageBoonUptime, Table::GetColumnWidth(), *entity);
			} else {
				buffProgressBar(trackedBuff, averageBoonUptime, Table::GetColumnWidth());
			}
		}
	}

	// above90
	if (Table::TableNextColumn()) {
		const float above90 = above90Func();
		if (hasEntity) {
			buffProgressBar(*above90BoonDef, above90, ImGui::GetColumnWidth(), *entity);
		} else {
			buffProgressBar(*above90BoonDef, above90, ImGui::GetColumnWidth());
		}
	}

	endOfRow();
}
*/

void AppChart::removePlayer(size_t playerId) {
    mTable->removePlayer(playerId);
}

void AppChart::addPlayer(size_t playerId) {
	mTable->addPlayer(playerId);
}

void AppChart::sortNeeded() {
	mTable->sortNeeded();
}

void AppChart::endOfRow() {
	if (rowCount < settings.getMaxDisplayed(index)) {
		innerTableCursorPos = ImGui::GetCursorPosY();
		// innerTableCursorPos = ImGui::GetCurrentWindow()->DC.CursorPosPrevLine.y;
	}

	++rowCount;
}

void AppChartsContainer::removePlayer(uintptr_t playerId) {
	for (AppChart& chart : charts) {
		chart.removePlayer(playerId);
	}
}

void AppChartsContainer::addPlayer(uintptr_t playerId) {
	for (AppChart& chart : charts) {
		chart.addPlayer(playerId);
	}
}

void AppChartsContainer::sortNeeded() {
	for (AppChart& chart : charts) {
		chart.sortNeeded();
	}
}

void AppChartsContainer::drawAll(ImGuiWindowFlags flags) {
	PRINT_LINE()
	for (AppChart& chart : charts) {
		chart.Draw(flags);
	}
}
