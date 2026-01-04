#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <chrono>

#include "TrackerHistory.h"
#include <ArcdpsExtension/arcdps_structs_slim.h>

class History {
private:
	using EntryType = std::deque<TrackerHistory>;
	// this holds the last X of fights as history
	EntryType entries;
	std::mutex entriesMutex;
	std::mutex historyMutex;

public:
	void LogStart(cbtevent* event);
	void LogEnd(cbtevent* event);
	void Event(ag* dst);
	void Reset(cbtevent* event);

	// Requires a call to lock() for the whole duration of the usage of the return value.
	std::optional<size_t> GetTrackerIndexById(uint64_t trackerId, std::unique_lock<std::mutex>& lock);
	// Requires a call to lock() for the whole duration of the usage of the return value.
	[[nodiscard]] EntryType::const_iterator begin(std::unique_lock<std::mutex>& lock) const {
		assert(lock.owns_lock() && lock.mutex() == &entriesMutex);
		return entries.begin();
	}
	// Requires a call to lock() for the whole duration of the usage of the return value.
	[[nodiscard]] EntryType::const_iterator end(std::unique_lock<std::mutex>& lock) const {
		assert(lock.owns_lock() && lock.mutex() == &entriesMutex);
		return entries.end();
	}
	// Requires a call to lock() for the whole duration of the usage of the return value.
	EntryType::iterator begin(std::unique_lock<std::mutex>& lock) {
		assert(lock.owns_lock() && lock.mutex() == &entriesMutex);
		return entries.begin();
	}
	// Requires a call to lock() for the whole duration of the usage of the return value.
	EntryType::iterator end(std::unique_lock<std::mutex>& lock) {
		assert(lock.owns_lock() && lock.mutex() == &entriesMutex);
		return entries.end();
	}
	// Requires a call to lock() for the whole duration of the usage of the return value.
	TrackerHistory& at(EntryType::size_type val, std::unique_lock<std::mutex>& lock) {
		assert(lock.owns_lock() && lock.mutex() == &entriesMutex);
		return entries.at(val);
	}

	// Locks the entries mutex and returns the lock object. This must be called before all methods marked as such.
	std::unique_lock<std::mutex> lock() {
		return std::unique_lock<std::mutex>(entriesMutex);
	}
	// Returns a lock object for the entries mutex not yet locked. The lock must be acquired by the caller before all methods marked as such.
	std::unique_lock<std::mutex> lock(std::defer_lock_t defer_lock) {
		return std::unique_lock<std::mutex>(entriesMutex, defer_lock);
	}

private:
	enum class Status {
		Empty,
		WaitingForName,
		WaitingForReset,
		NameAcquired
	};
	uintptr_t currentID = 0;
	Status status = Status::Empty;
	std::string currentName;
	uint32_t currentBeginTimestamp = 0;
	std::chrono::hh_mm_ss<std::chrono::system_clock::duration> beginTimestamp;
	uint64_t trackerIdCounter = 0;
};

extern History history;
