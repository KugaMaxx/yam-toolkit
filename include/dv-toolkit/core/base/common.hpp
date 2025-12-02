#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <vector>

#include <dv-processing/core/concepts.hpp>
#include <dv-processing/core/dvassert.hpp>
#include <dv-processing/core/time.hpp>
#include <dv-processing/core/time_window.hpp>

namespace dv::toolkit {

/**
 * @brief 
 * 
 * @tparam Type 
 */
template<class Type> // requires dv::Event || dv::Frame || dv::IMU
struct Packet : public flatbuffers::NativeTable {
public:
	std::vector<Type> elements;

	Packet() {
    }

	Packet(const std::vector<Type> &_elements) : elements{_elements} {
	}
	
	friend std::ostream &operator<<(std::ostream &os, const Packet &packet) {
		if (packet.elements.empty()) {
			os << fmt::format("Packet is empty!");
			return os;
		}

		int64_t lowestTime  = 0;
		int64_t highestTime = 0;

		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			lowestTime  = packet.elements.front().timestamp();
			highestTime = packet.elements.back().timestamp();
		} else {
			lowestTime  = packet.elements.front().timestamp;
			highestTime = packet.elements.back().timestamp;
		}

		os << fmt::format("Packet containing {} elements within {}μs duration; time range within [{}; {}]",
			packet.elements.size(), highestTime - lowestTime, lowestTime, highestTime);
		return os;
	}
};

/**
 * @brief 
 * 
 * @tparam Type 
 */
template<typename Type> // dv::concepts::TimestampedByAccessor
class TimeComparator {
public:
	bool operator()(const Type &element, const int64_t time) const {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			return element.timestamp() < time;
		} else {
			return element.timestamp < time;
		}
	}

	bool operator()(const int64_t time, const Type &element) const {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			return time < element.timestamp();
		} else {
			return time < element.timestamp;
		}
	}
};

/**
 * @brief 
 * 
 * @tparam Type 
 * @tparam PacketType 
 */
template<typename Type, class PacketType>
class PartialData {
    using iterator = typename std::vector<Type>::const_iterator;
    
private:
	bool referencesConstData_;
	size_t start_;
	size_t length_;
	size_t capacity_;
	int64_t lowestTime_;
	int64_t highestTime_;
	std::shared_ptr<PacketType> modifiableDataPtr_;
	std::shared_ptr<const PacketType> data_;

public:
	explicit PartialData(const size_t capacity = 10000) :
		referencesConstData_(false),
		start_(0),
		length_(0),
		capacity_(capacity),
		lowestTime_(0),
		highestTime_(0),
		modifiableDataPtr_(std::make_shared<PacketType>()),
		data_(modifiableDataPtr_) {
		modifiableDataPtr_->elements.reserve(capacity);
	}

	explicit PartialData(std::shared_ptr<const PacketType> data) :
		referencesConstData_(true),
		start_(0),
		length_(data->elements.size()),
		capacity_(length_),
		modifiableDataPtr_(nullptr),
		data_(data) {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			lowestTime_ = data->elements.front().timestamp();
			highestTime_ = data->elements.back().timestamp();
		} else {
			lowestTime_ = data->elements.front().timestamp;
			highestTime_ = data->elements.back().timestamp;
		}
	}

    PartialData(const PartialData &other) = default;

	iterator iteratorAtTime(const int64_t time) const {
		auto comparator    = TimeComparator<Type>();
		auto sliceItr = std::lower_bound(begin(), end(), time, comparator);
		return sliceItr;
	}

	iterator begin() const {
		return data_->elements.begin() + start_;
	}

	iterator end() const {
		return data_->elements.begin() + start_ + length_;
	}

	void sliceFront(const size_t number) {
		if (number > length_) {
			throw std::range_error("Can not slice more than length from PartialData.");
		}

		start_      = start_ + number;
		length_     = length_ - number;
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			lowestTime_ = (length_ == 0) ? (0) : (data_->elements)[start_].timestamp();
		} else {
			lowestTime_ = (length_ == 0) ? (0) : (data_->elements)[start_].timestamp;
		}
	}

	void sliceBack(const size_t number) {
		if (number > length_) {
			throw std::range_error("Can not slice more than length from PartialData.");
		}

		length_      = length_ - number;
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			highestTime_ = (length_ == 0) ? (0) : (data_->elements)[start_ + length_ - 1].timestamp();
		} else {
			highestTime_ = (length_ == 0) ? (0) : (data_->elements)[start_ + length_ - 1].timestamp;
		}
	}

	size_t sliceTimeFront(const int64_t time) {
		auto timeItr = iteratorAtTime(time);
		auto index   = static_cast<size_t>(timeItr - begin());
		sliceFront(index);
		return index;
	}

	size_t sliceTimeBack(const int64_t time) {
		auto timeItr     = iteratorAtTime(time);
		auto index       = static_cast<size_t>(timeItr - begin());
		size_t cutAmount = length_ - index;
		sliceBack(cutAmount);
		return cutAmount;
	}

	void _unsafe_add(const Type &element) {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			highestTime_ = element.timestamp();
			if (length_ == 0) {
				lowestTime_ = element.timestamp();
			}
		} else {
			highestTime_ = element.timestamp;
			if (length_ == 0) {
				lowestTime_ = element.timestamp;
			}
		}
		
		modifiableDataPtr_->elements.emplace_back(element);
		length_++;
	}

	void _unsafe_move(Type &&element) {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			highestTime_ = element.timestamp();
			if (length_ == 0) {
				lowestTime_ = element.timestamp();
			}
		} else {
			highestTime_ = element.timestamp;
			if (length_ == 0) {
				lowestTime_ = element.timestamp;
			}
		}

		// this should cause a move action instead of a copy
		modifiableDataPtr_->elements.push_back(std::move(element));
		length_++;
	}

	[[nodiscard]] Type &front() {
		return *modifiableDataPtr_->elements[start_];
	}

	[[nodiscard]] Type &back() {
		return modifiableDataPtr_->elements[start_ + length_ - 1];
	}

	[[nodiscard]] inline size_t getLength() const {
		return length_;
	}

	[[nodiscard]] inline int64_t getLowestTime() const {
		return lowestTime_;
	}

	[[nodiscard]] inline int64_t getHighestTime() const {
		return highestTime_;
	}

	[[nodiscard]] inline const Type &operator[](size_t offset) const {
		dv::runtime_assert([&] { return offset <= length_; }, [] { return "offset out of bounds"; });
		return (data_->elements)[start_ + offset];
	}

	[[nodiscard]] inline bool canStoreMore() const {
		return !referencesConstData_
			&& (data_->elements.size() < capacity_ && start_ + length_ == data_->elements.size());
	}

	[[nodiscard]] inline size_t availableCapacity() const {
		if (referencesConstData_) {
			return 0;
		}

		return capacity_ - data_->elements.size();
	}

	[[nodiscard]] bool merge(const PartialData &other) {
		// Test whether merge is allowed.
		if (!canStoreMore() || availableCapacity() < other.getLength()) {
			return false;
		}

		// If the other shard is empty, we do not have to do anything
		if (other.getLength() == 0) {
			return true;
		}

		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			highestTime_ = back().timestamp();
		} else {
			highestTime_ = back().timestamp;
		}

		// Actual merge
		modifiableDataPtr_->elements.insert(end(), other.begin(), other.end());
		length_      += other.length_;
		return true;
	}
};


/**
 * @brief 
 * 
 * @tparam Type 
 * @tparam PacketType 
 */
template<typename Type, class PacketType>
class PartialDataTimeComparator {
private:
	const bool lower_;

public:
	explicit PartialDataTimeComparator(const bool lower) : lower_(lower) {
	}

	bool operator()(const PartialData<Type, PacketType> &partial, const int64_t time) const {
		return lower_ ? partial.getLowestTime() < time : partial.getHighestTime() < time;
	}

	bool operator()(const int64_t time, const PartialData<Type, PacketType> &partial) const {
		return lower_ ? time < partial.getLowestTime() : time < partial.getHighestTime();
	}
};


/**
 * @brief 
 * 
 * @tparam Type 
 * @tparam PacketType 
 */
template<typename Type, class PacketType>
class AddressableStorageIterator {
private:
	const std::vector<PartialData<Type, PacketType>> *dataPartialsPtr_;
	/** The current partial (shard) we point to */
	size_t partialIndex_;
	/** The current offset inside the shard we point to */
	size_t offset_;

	inline void increment() {
		offset_++;
		if (offset_ >= (*dataPartialsPtr_)[partialIndex_].getLength()) {
			offset_ = 0;
			if (partialIndex_ < dataPartialsPtr_->size()) { // increment only to one partial after end
				partialIndex_++;
			}
		}
	}

	inline void decrement() {
		if (partialIndex_ >= dataPartialsPtr_->size()) {
			partialIndex_ = dataPartialsPtr_->size() - 1;
			offset_       = (*dataPartialsPtr_)[partialIndex_].getLength() - 1;
		}
		else {
			if (offset_ > 0) {
				offset_--;
			}
			else {
				if (partialIndex_ > 0) {
					partialIndex_--;
					offset_ = (*dataPartialsPtr_)[partialIndex_].getLength() - 1;
				}
			}
		}
	}

public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type        = const Type;
	using pointer           = const Type *;
	using reference         = const Type &;
	using size_type         = size_t;
	using difference_type   = ptrdiff_t;

	AddressableStorageIterator() : AddressableStorageIterator(nullptr, true) {
	}

	explicit AddressableStorageIterator(
		const std::vector<PartialData<Type, PacketType>> *dataPartialsPtr, const bool front) :
		dataPartialsPtr_(dataPartialsPtr),
		offset_(0) {
		partialIndex_ = front ? 0 : dataPartialsPtr->size();
	}

	AddressableStorageIterator(
		const std::vector<PartialData<Type, PacketType>> *dataPartialsPtr,
		const size_t partialIndex, const size_t offset) :
		dataPartialsPtr_(dataPartialsPtr),
		partialIndex_(partialIndex),
		offset_(offset) {
	}

	inline const Type &operator*() const noexcept {
		return (*dataPartialsPtr_)[partialIndex_][offset_];
	}

	inline const Type *operator->() const noexcept {
		return &(this->operator*());
	}
	
	AddressableStorageIterator &operator++() noexcept {
		increment();
		return *this;
	}

	const AddressableStorageIterator operator++(int) noexcept {
		auto currentIterator = AddressableStorageIterator(dataPartialsPtr_, partialIndex_, offset_);
		increment();
		return currentIterator;
	}

	AddressableStorageIterator &operator+=(const size_type add) noexcept {
		for (size_t i = 0; i < add; i++) {
			increment();
		}
		return *this;
	}

	AddressableStorageIterator &operator--() noexcept {
		decrement();
		return *this;
	}

	const AddressableStorageIterator operator--(int) noexcept {
		auto currentIterator = AddressableStorageIterator(dataPartialsPtr_, partialIndex_, offset_);
		decrement();
		return currentIterator;
	}

	AddressableStorageIterator &operator-=(const size_type sub) noexcept {
		for (size_t i = 0; i < sub; i++) {
			decrement();
		}
		return *this;
	}

	bool operator==(const AddressableStorageIterator &rhs) const noexcept {
		return (partialIndex_ == rhs.partialIndex_) && (offset_ == rhs.offset_);
	}

	bool operator!=(const AddressableStorageIterator &rhs) const noexcept {
		return !(this->operator==(rhs));
	}
};

/**
 * @brief 将packet封装为具有各种功能的storage
 * 
 * @tparam Type 
 * @tparam PacketType 
 */
template<typename Type, class PacketType, class StorageType>
class AddressableStorage {
public:
	// Container traits.
	using value_type         = Type;
	using const_value_type   = const Type;
	using pointer            = Type *;
	using const_pointer      = const Type *;
	using reference          = Type &;
	using const_reference    = const Type &;
	using size_type          = size_t;
	using difference_type    = ptrdiff_t;
	using packet_type 		 = PacketType;
	using const_packet_type	 = const PacketType;
	using storage_type		 = StorageType;
	using const_storage_type = const StorageType;

	// Intrinsic traits
	using PartialDataType = PartialData<Type, PacketType>;
	using iterator        = AddressableStorageIterator<Type, PacketType>;
	using const_iterator  = iterator;

protected:
	/** internal list of the shards. */
	std::vector<PartialDataType> dataPartials_;
	/** The exact number-of-elements global offsets of the shards */
	std::vector<size_t> partialOffsets_;
	/** The total length of the element package */
	size_t totalLength_{0};
	/** Default capacity for the data partials **/
	size_t shardCapacity_{10000};

	explicit AddressableStorage(const std::vector<PartialDataType> &dataPartials) {
		this->dataPartials_ = dataPartials;

		// Build up length and offsets
		for (const auto &partial : dataPartials) {
			partialOffsets_.emplace_back(totalLength_);
			totalLength_ += partial.getLength();
		}
	}

	[[nodiscard]] PartialData<Type, PacketType> &_getLastNonFullPartial() {
		if (!dataPartials_.empty() && dataPartials_.back().canStoreMore()) {
			return dataPartials_.back();
		}

		partialOffsets_.emplace_back(totalLength_);
		return dataPartials_.emplace_back(shardCapacity_);
	}

public:
	AddressableStorage() = default;

	explicit AddressableStorage(const PacketType &packet) :
		AddressableStorage(std::make_shared<PacketType>(packet)) {
	}

	explicit AddressableStorage(std::shared_ptr<const PacketType> packet) {
		if (!packet) {
			return;
		}

		if (packet->elements.empty()) {
			return;
		}

		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			if (!dataPartials_.empty() && dataPartials_.back().getHighestTime() > packet->elements.front().timestamp()) {
				throw std::out_of_range{"Tried adding element packet to store out of order."};
			}
		} else {
			if (!dataPartials_.empty() && dataPartials_.back().getHighestTime() > packet->elements.front().timestamp) {
				throw std::out_of_range{"Tried adding element packet to store out of order."};
			}
		}

		dataPartials_.emplace_back(packet);
		partialOffsets_.emplace_back(totalLength_);
		totalLength_ += dataPartials_.back().getLength();
	}

	void add(const StorageType &store) {
		if (store.isEmpty()) {
			return;
		}

		if (getHighestTime() > store.getLowestTime()) {
			throw std::out_of_range{"Tried adding elements to store out of order."};
		}

		for (const auto &partial : store.dataPartials_) {
			// Try to merge with the last partial if possible
			if (dataPartials_.empty() || !dataPartials_.back().merge(partial)) {
				// If we have no dataPartials yet or merging with the last partial fails, just append the new partial
				// to the partial list
				dataPartials_.push_back(partial);
				partialOffsets_.push_back(totalLength_);
			}

			totalLength_ += partial.getLength();
		}
	}

	StorageType &operator=(std::shared_ptr<const PacketType> packet) {
		*this = StorageType{std::move(packet)};
		return *this;
	}

	[[nodiscard]] inline StorageType operator+(const StorageType &other) const {
		auto store = StorageType(*this);
		store.add(other);
		return store;
	}

	inline void operator+=(const StorageType &other) {
		add(other);
	}

	inline StorageType &operator<<(const Type &element) {
		push_back(element);
		return *this;
	}

	[[nodiscard]] StorageType slice(const size_t start) const {
		if (totalLength_ == 0 || start >= totalLength_) {
			return StorageType();
		}

		return slice(start, totalLength_ - start);
	}

	[[nodiscard]] StorageType slice(
		const size_t start, const size_t length) const {
		if (start + length > totalLength_) {
			throw std::range_error("Slice exceeds Store range");
		}

		if (length == 0) {
			return StorageType();
		}

		std::vector<PartialDataType> newPartials;
		auto lowerPartial = std::upper_bound(partialOffsets_.begin(), partialOffsets_.end(), start);
		auto upperPartial = std::lower_bound(partialOffsets_.begin(), partialOffsets_.end(), start + length);
		auto lowIndex     = static_cast<size_t>(lowerPartial - partialOffsets_.begin()) - 1;
		auto highIndex    = static_cast<size_t>(upperPartial - partialOffsets_.begin());
		for (size_t i = lowIndex; i < highIndex; i++) {
			newPartials.emplace_back(dataPartials_[i]);
		}
		size_t frontSliceAmount = start - partialOffsets_[lowIndex];
		size_t backSliceAmount  = partialOffsets_[highIndex - 1] + newPartials.back().getLength() - (start + length);
		newPartials.front().sliceFront(frontSliceAmount);
		newPartials.back().sliceBack(backSliceAmount);

		if (newPartials.front().getLength() <= 0) {
			newPartials.erase(newPartials.begin());
		}

		if (newPartials.back().getLength() <= 0) {
			newPartials.erase(newPartials.end() - 1);
		}
		return StorageType(newPartials);
	}

	[[nodiscard]] StorageType sliceBack(const size_t length) const {
		if (length >= totalLength_) {
			return slice(0);
		}
		else {
			return slice(totalLength_ - length, length);
		}
	}

	[[nodiscard]] StorageType sliceTime(const int64_t startTime) const {
		int64_t s = startTime < 0 ? (getHighestTime() + startTime) : startTime;
		return sliceTime(s, getHighestTime() + 1); // + 1 to include the events that happen at the last time.
	}

	[[nodiscard]] StorageType sliceTime(
		const int64_t startTime, const int64_t endTime) const {
		size_t refStart = 0;
		size_t refEnd 	= 0;
		return sliceTime(startTime, endTime, refStart, refEnd);
	}

	[[nodiscard]] StorageType sliceTime(
		const int64_t startTime, const int64_t endTime, size_t &retStart, size_t &retEnd) const {
		// we find the relevant partials and slice the first and last one to fit
		std::vector<PartialDataType> newPartials;

		auto lowerPartial = std::lower_bound(dataPartials_.begin(), dataPartials_.end(), startTime,
			PartialDataTimeComparator<Type, PacketType>(false));
		auto upperPartial = std::lower_bound(dataPartials_.begin(), dataPartials_.end(), endTime,
			PartialDataTimeComparator<Type, PacketType>(true));

		size_t newLength = 0;
		for (auto it = lowerPartial; it < upperPartial; it++) {
			newLength += it->getLength();
			newPartials.emplace_back(*it);
		}

		if (newLength == 0) {
			// We are returning an empty slice, return indices are set to zeros
			retStart = 0;
			retEnd   = 0;

			return StorageType();
		}

		size_t cutFront = newPartials.front().sliceTimeFront(startTime);
		size_t cutBack  = newPartials.back().sliceTimeBack(endTime);
		newLength       = newLength - cutFront - cutBack;

		if (newPartials.front().getLength() <= 0) {
			newPartials.erase(newPartials.begin());
		}

		if (!newPartials.empty() && newPartials.back().getLength() <= 0) {
			newPartials.erase(newPartials.end() - 1);
		}

		retStart = partialOffsets_[static_cast<size_t>(lowerPartial - dataPartials_.begin())] + cutFront;
		retEnd   = retStart + newLength;

		return StorageType(newPartials);
	}

	[[nodiscard]] StorageType downSample(const size_t factor) {
		if (totalLength_ == 0) {
			return StorageType();
		}

		PartialDataType newPartial;
		for (size_t i = 0; i < totalLength_; i++) {
			if (i % factor == 0) {
				newPartial._unsafe_add(this->at(i));
			}
		}

		return StorageType(std::vector<PartialDataType>{newPartial});
	}

	void push_back(const Type &element) {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			if (getHighestTime() > element.timestamp()) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		} else {
			if (getHighestTime() > element.timestamp) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		}

		_getLastNonFullPartial()._unsafe_add(element);
		this->totalLength_++;
	}

	void push_back(Type &&element) {
		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			if (getHighestTime() > element.timestamp()) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		} else {
			if (getHighestTime() > element.timestamp) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		}

		_getLastNonFullPartial()._unsafe_move(std::move(element));
		this->totalLength_++;
	}

	template<class... Args>
	Type &emplace_back(Args &&...args) {
		Type element(std::forward<Args>(args)...);

		if constexpr (dv::concepts::TimestampedByAccessor<Type>) {
			if (getHighestTime() > element.timestamp()) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		} else {
			if (getHighestTime() > element.timestamp) {
				throw std::out_of_range{"Tried adding element to store out of order."};
			}
		}

		// This shouldn't cause any copy operations
		auto &targetPartial = _getLastNonFullPartial();
		targetPartial._unsafe_move(std::move(element));
		this->totalLength_++;

		return targetPartial.back();
	}

	[[nodiscard]] inline StorageType copy() const {
		return StorageType(dataPartials_);
	}

	[[nodiscard]] inline size_t size() const {
		return this->totalLength_;
	}

	[[nodiscard]] inline int64_t getLowestTime() const {
		if (isEmpty()) {
			return 0;
		}
		return dataPartials_.front().getLowestTime();
	}

	[[nodiscard]] inline int64_t getHighestTime() const {
		if (isEmpty()) {
			return 0;
		}
		return dataPartials_.back().getHighestTime();
	}

	[[nodiscard]] inline bool isEmpty() const {
		return totalLength_ == 0;
	}

	void erase(const size_t start, const size_t length) {
		if (start > totalLength_) {
			throw std::out_of_range("First index is beyond the size of the store");
		}

		if ((start + length) > totalLength_) {
			throw std::out_of_range("Erase range exceeds tore range");
		}

		if (length == 0) {
			return;
		}

		// Find everything for indexing
		const auto lowerPartial = std::upper_bound(partialOffsets_.begin(), partialOffsets_.end(), start);
		auto upperPartial       = std::lower_bound(partialOffsets_.begin(), partialOffsets_.end(), start + length);
		const auto lowIndex     = static_cast<size_t>(std::distance(partialOffsets_.begin(), lowerPartial) - 1);
		const auto highIndex    = static_cast<size_t>(std::distance(partialOffsets_.begin(), upperPartial));
		auto lowerIter          = std::next(dataPartials_.begin(), static_cast<ptrdiff_t>(lowIndex));
		auto upperIter          = std::next(dataPartials_.begin(), static_cast<ptrdiff_t>(highIndex));

		if ((highIndex - lowIndex) == 1) {
			// We are dealing with changes within a single partial

			// If we are dealing within the last packet, partial offsets will not contain
			// the value for it
			size_t highLimit;
			if (highIndex < partialOffsets_.size()) {
				highLimit = partialOffsets_[highIndex];
			}
			else {
				highLimit = totalLength_;
			}

			if (start == partialOffsets_[lowIndex]) {
				// Erase off front
				lowerIter->sliceFront(std::min(length, lowerIter->getLength()));
			}
			else if ((start + length) == highLimit) {
				// Erase off back
				lowerIter->sliceBack(std::min(length, lowerIter->getLength()));
			}
			else if ((start + length) < highLimit) {
				// Erase in between. Make a copy of the packet and erase back of first
				// and erase front of second.
				const size_t correctedStart = (start - partialOffsets_[lowIndex]);

				PartialData copy = *lowerIter;
				copy.sliceFront(correctedStart + length);

				lowerIter->sliceBack(lowerIter->getLength() - correctedStart);
				upperIter = dataPartials_.insert(lowerIter + 1, copy);
				lowerIter = std::next(dataPartials_.begin(), static_cast<ptrdiff_t>(lowIndex));
			}

			// Remove the empty partials if that is required
			if (lowerIter->getLength() <= 0) {
				lowerIter = dataPartials_.erase(lowerIter);
			}
			else {
				lowerIter++;
			}
		}
		else {
			// Erase elements spanning within several partials
			if (start > partialOffsets_[lowIndex]) {
				lowerIter->sliceBack(lowerIter->getLength() - (start - partialOffsets_[lowIndex]));
				lowerIter++;
			}

			// Go to the last affected partial and delete
			upperIter--;
			upperPartial--;

			const size_t upperBackSliceAmount = std::min((start + length) - *upperPartial, upperIter->getLength());
			upperIter->sliceFront(upperBackSliceAmount);

			if (upperIter->getLength() <= 0) {
				upperIter = dataPartials_.erase(upperIter);
			}
		}

		// Erase the data in between the bounds
		if (!dataPartials_.empty() && lowerIter != upperIter) {
			dataPartials_.erase(lowerIter, upperIter);
		}

		// Rebuild the partials offset LUT
		partialOffsets_.erase(lowerPartial, partialOffsets_.end());
		totalLength_ = partialOffsets_.back();

		auto dataIter = std::next(dataPartials_.begin(), static_cast<ptrdiff_t>(partialOffsets_.size() - 1));
		partialOffsets_.reserve(dataPartials_.size());

		while (dataIter != dataPartials_.end()) {
			totalLength_ += dataIter->getLength();
			partialOffsets_.emplace_back(totalLength_);
			dataIter++;
		}
	}

	size_t eraseTime(const int64_t startTime, const int64_t endTime) {
		if (startTime > endTime) {
			throw std::invalid_argument("Start time is greater than end time in eraseTime function call");
		}

		const auto lowerPartial = std::lower_bound(dataPartials_.begin(), dataPartials_.end(), startTime,
			PartialDataTimeComparator<Type, PacketType>(false));
		auto upperPartial       = std::lower_bound(dataPartials_.begin(), dataPartials_.end(), endTime,
				  PartialDataTimeComparator<Type, PacketType>(true));

		if (upperPartial == dataPartials_.end()) {
			upperPartial--;
		}

		const auto cutFront
			= static_cast<size_t>(std::distance(lowerPartial->begin(), lowerPartial->iteratorAtTime(startTime)));
		const auto cutBack
			= static_cast<size_t>(std::distance(upperPartial->begin(), upperPartial->iteratorAtTime(endTime)));
		const auto partialIndexStart = static_cast<size_t>(std::distance(dataPartials_.begin(), lowerPartial));
		const auto partialIndexEnd   = static_cast<size_t>(std::distance(dataPartials_.begin(), upperPartial));

		const auto eraseIndexStart = partialOffsets_[partialIndexStart] + cutFront;
		const auto eraseLength     = (partialOffsets_[partialIndexEnd] + cutBack) - eraseIndexStart;
		erase(eraseIndexStart, eraseLength);

		return eraseLength;
	}

	[[nodiscard]] const_iterator begin() const noexcept {
		return (iterator(&dataPartials_, true));
	}

	[[nodiscard]] const_iterator end() const noexcept {
		return (iterator(&dataPartials_, false));
	}

	[[nodiscard]] const Type & front() const {
		return *iterator(&dataPartials_, true);
	}

	[[nodiscard]] const Type & back() const {
		iterator it(&dataPartials_, false);
		it -= 1;
		return *it;
	}

	[[nodiscard]] const Type &operator[](const size_t index) const {
		dv::runtime_assert([&] { return index < totalLength_; }, [] { return "Index exceeds Store range"; });

		const auto lowerPartial = std::upper_bound(partialOffsets_.begin(), partialOffsets_.end(), index);
		const auto lowIndex     = static_cast<size_t>(std::distance(partialOffsets_.begin(), lowerPartial) - 1);
		
		return dataPartials_[lowIndex][index - partialOffsets_[lowIndex]];
	}

	[[nodiscard]] const Type &at(const size_t index) const {
		dv::runtime_assert([&] { return index < totalLength_; }, [] { return "Index exceeds Store range"; });

		const auto lowerPartial = std::upper_bound(partialOffsets_.begin(), partialOffsets_.end(), index);
		const auto lowIndex     = static_cast<size_t>(std::distance(partialOffsets_.begin(), lowerPartial) - 1);

		return dataPartials_[lowIndex][index - partialOffsets_[lowIndex]];
	}

	void retainDuration(const dv::Duration duration) {
		const auto startTime = getHighestTime() - duration.count();
		auto lowerPartial    = std::lower_bound(dataPartials_.begin(), dataPartials_.end(), startTime,
			   PartialDataTimeComparator<Type, PacketType>(false));

		if (lowerPartial != dataPartials_.begin()) {
			dataPartials_.erase(dataPartials_.begin(), --lowerPartial);
			partialOffsets_.clear();
			partialOffsets_.reserve(dataPartials_.size());
			totalLength_ = 0;

			for (const auto &partial : dataPartials_) {
				partialOffsets_.push_back(totalLength_);
				totalLength_ += partial.getLength();
			}
		}
	}

	[[nodiscard]] dv::TimeWindow timeWindow() const {
		return dv::TimeWindow(getLowestTime(), getHighestTime());
	}

	[[nodiscard]] dv::Duration duration() const {
		return dv::Duration(getHighestTime() - getLowestTime());
	}

	[[nodiscard]] bool isWithinStoreTimeRange(const int64_t timestamp) const {
		return timestamp >= getLowestTime() && timestamp <= getHighestTime();
	}

	[[nodiscard]] double rate() const {
		const int64_t durationMicros = getHighestTime() - getLowestTime();
		if (durationMicros == 0) {
			return 0.;
		}
		return static_cast<double>(size()) / (static_cast<double>(durationMicros) * 1e-6);
	}

	[[nodiscard]] size_t getShardCapacity() const {
		return shardCapacity_;
	}

	void setShardCapacity(const size_t shardCapacity) {
		shardCapacity_ = std::max<size_t>(1ULL, shardCapacity);
	}

	friend std::ostream &operator<<(std::ostream &os, const StorageType &storage) {
		if (storage.size() == 0) {
		os << fmt::format("Storage is empty!",
			storage.size(), storage.duration(), storage.getLowestTime(), storage.getHighestTime());
		return os;
		}

		os << fmt::format("Storage containing {} elements within {} duration; time range within [{}; {}]",
			storage.size(), storage.duration(), storage.getLowestTime(), storage.getHighestTime());
		return os;
	}
};

} // namespace dv::toolkit
