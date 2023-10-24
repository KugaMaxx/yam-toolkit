#pragma once

#include "./core.hpp"

namespace dv::toolkit {

// add template requires
template<class DataType>
class DataSlicer {
protected:
    class SliceJob{
		using JobCallback = std::function<void(const dv::TimeWindow &, const DataType &)>;

	private:
		DataType mData;
        std::string  mReference;
		JobCallback  mCallback;
		int64_t mTimeInterval 	= -1;
		int64_t mLastCallTime 	= 0;
		size_t 	mNumberInterval = 0;
		size_t 	mLastCallNumber = 0;

    public:
        enum class SliceType {
            NUMBER,
            TIME
        } mType;

        SliceJob() = default;

		SliceJob(
			const std::string & name, const SliceType type, 
            const int64_t timeInterval, const size_t numberInterval, JobCallback callback) :
			mReference(name),
            mType(type),
			mCallback(std::move(callback)),
			mTimeInterval(timeInterval),
			mNumberInterval(numberInterval),
			mLastCallTime(0),
			mLastCallNumber(0) {
		}

		void run(const DataType &data) {
			if (data.size(mReference) == 0) {
				return;
			}

			mData.add(data);
			if (mLastCallTime == 0) {
				mLastCallTime = mData.timeWindow(mReference).startTime;
			}

			if (mType == SliceType::NUMBER) {
				while (mData.size(mReference) - mLastCallNumber >= mNumberInterval) {
					DataType slice;
					slice = mData.sliceByNumber(mReference, mLastCallNumber, mNumberInterval);
					mLastCallTime	= slice.timeWindow(mReference).endTime;
					mLastCallNumber = mLastCallNumber + mNumberInterval;
					mCallback(slice.timeWindow(mReference), slice);
				}
			}

			if (mType == SliceType::TIME) {
				while (mData.timeWindow(mReference).endTime - mLastCallTime >= mTimeInterval) {
					DataType slice;
					slice = mData.sliceByTime(mReference, mLastCallTime, mLastCallTime + mTimeInterval);
					mLastCallTime = mLastCallTime + mTimeInterval;
					mLastCallNumber = mLastCallNumber + slice.size(mReference);
					mCallback(slice.timeWindow(mReference), slice);
				}
			}

			size_t length = mData.size(mReference) - mLastCallNumber;
			mData = mData.sliceByNumber(mReference, mLastCallNumber, length);
			mLastCallNumber = 0;
		}

		void setTimeInterval(const int64_t timeInterval) {
			if (mType != SliceType::TIME) {
				throw std::invalid_argument("Setting a new number interval to a time based slicing job");
			}
			mTimeInterval = timeInterval;
		}

		void setNumberInterval(const size_t numberInterval) {
			if (mType != SliceType::NUMBER) {
				throw std::invalid_argument("Setting a new time interval to a number based slicing job");
			}
			mNumberInterval = numberInterval;
		}
    };

private:
    int32_t mHashCounter = 0;
	std::map<int, SliceJob> mSliceJobs;

public:
    DataSlicer() = default;

	void accept(const DataType &data) {
		for (auto &jobTuple : mSliceJobs) {
			jobTuple.second.run(data);
		}
	}

	int doEveryNumberOfElements(
		const std::string &name, const size_t n,
		std::function<void(const DataType &)> callback) {
		return doEveryNumberOfElements(name, n, [callback](const dv::TimeWindow &, const DataType &data) {
			callback(data);
		});
	}

	int doEveryNumberOfElements(
		const std::string &name, const size_t n,
		std::function<void(const dv::TimeWindow &, const DataType &)> callback) {
		mHashCounter += 1;
		mSliceJobs.emplace(std::make_pair(
			mHashCounter, SliceJob(name, SliceJob::SliceType::NUMBER, 0, n, callback)));
		return mHashCounter;
	}

	int doEveryTimeInterval(
		const std::string &name, const dv::Duration interval, 
		std::function<void(const DataType &)> callback) {
		return doEveryTimeInterval(name, interval, [callback](const dv::TimeWindow &, const DataType &data) {
			callback(data);
		});
	}

	int doEveryTimeInterval(
		const std::string &name, const dv::Duration interval, 
        std::function<void(const dv::TimeWindow &, const DataType &)> callback) {
        mHashCounter += 1;
		mSliceJobs.emplace(std::make_pair(
            mHashCounter, SliceJob(name, SliceJob::SliceType::TIME, interval.count(), 0, callback)));
        return mHashCounter;
    }

	[[nodiscard]] bool hasJob(const int jobId) const {
		return mSliceJobs.contains(jobId);
	}

	void removeJob(const int jobId) {
		if (!hasJob(jobId)) {
			return;
		}
		mSliceJobs.erase(jobId);
	}

	void modifyTimeInterval(const int jobId, const dv::Duration timeInterval) {
		if (!hasJob(jobId)) {
			return;
		}
		mSliceJobs[jobId].setTimeInterval(timeInterval.count());
	}

	void modifyNumberInterval(const int jobId, const size_t numberInterval) {
		if (!hasJob(jobId)) {
			return;
		}
		mSliceJobs[jobId].setNumberInterval(numberInterval);
	}
};

using MonoCameraSlicer = DataSlicer<MonoCameraData>;

} // namespace dv::toolkit
