#ifndef LIBFILEZILLA_TIME_HEADER
#define LIBFILEZILLA_TIME_HEADER

#include "libfilezilla.hpp"

#include <chrono>
#include <ctime>

#include <limits>

#ifdef FZ_WINDOWS
#include "private/windows.hpp"
#endif

namespace fz {

class FZ_PUBLIC_SYMBOL duration;

/**
\brief Represents a point of time in wallclock with the given accuracy/precision.

As time may come from different sources that have different accuracy/precision,
this class keeps track of accuracy information.

The \ref compare function can be used for accuracy-aware comparisons. Conceptually it works
as if naively comparing both timestamps after truncating/clamping them to the least common accuracy.

Internal representation is in milliseconds since 1970-01-01 00:00:00.000 UTC. While datetime
supports negative times (i.e. earlier than 1970-01-01 00:00:00.000 UTC), the underlying plaform
may not support it.

\remark Some *nix systems base their time on TAI instead of UTC, though we pretend there
is no difference, as the latter is the default on every modern distribution.
*/
class FZ_PUBLIC_SYMBOL datetime final
{
public:
	enum accuracy : char {
		days,
		hours,
		minutes,
		seconds,
		milliseconds
	};

	/**
	 * \brief When importing or exporting a timestamp, zone is used to explicitly specify whether the
	 * conversion is to/from localtime or UTC.
	 */
	enum zone {
		utc,
		local
	};

	datetime() = default;

	datetime(zone z, int year, int month, int day, int hour = -1, int minute = -1, int second = -1, int millisecond = -1);

	explicit datetime(time_t, accuracy a);

	// Parses string, looks for YYYYmmDDHHMMSSsss
	// Ignores all non-digit characters between fields.
	explicit datetime(std::string const& s, zone z);
	explicit datetime(std::wstring const& s, zone z);

#ifdef FZ_WINDOWS
	explicit datetime(FILETIME const& ft, accuracy a);
#endif

	datetime(datetime const& op) = default;
	datetime(datetime && op) noexcept = default;
	datetime& operator=(datetime const& op) = default;
	datetime& operator=(datetime && op) noexcept = default;

	bool empty() const;
	void clear();

	accuracy get_accuracy() const { return a_; }

	/// Returns the current date/time
	static datetime now();

	bool operator==(datetime const& op) const;
	bool operator!=(datetime const& op) const { return !(*this == op); }
	bool operator<(datetime const& op) const;
	bool operator<=(datetime const& op) const;
	bool operator>(datetime const& op) const { return op < *this; }

	int compare(datetime const& op) const;
	bool earlier_than(datetime const& op) const { return compare(op) < 0; };
	bool later_than(datetime const& op) const { return compare(op) > 0; };

	datetime& operator+=(duration const& op);
	datetime operator+(duration const& op) const { datetime t(*this); t += op; return t; }

	datetime& operator-=(duration const& op);
	datetime operator-(duration const& op) const { datetime t(*this); t -= op; return t; }

	friend duration operator-(datetime const& a, datetime const& b);

	// Beware: month and day are 1-indexed!
	bool set(zone z, int year, int month, int day, int hour = -1, int minute = -1, int second = -1, int millisecond = -1);

	bool set(std::string const& str, zone z);
	bool set(std::wstring const& str, zone z);

#ifdef FZ_WINDOWS
	/// Windows-only: Get set timestamp base on FILETIME
	bool set(FILETIME const& ft, accuracy a);
	/// Windows-only: Get set timestamp base on FILETIME
	bool set(SYSTEMTIME const& ft, accuracy a, zone z);
#else
	/** Sets timestamp from struct tm.
	 * \warning modifies passed structure
	 * \returns \c true on success, \c false on failure. The object's value is undefined after failure.
	 */
	bool set(tm & t, accuracy a, zone z);
#endif

	bool imbue_time(int hour, int minute, int second = -1, int millisecond = -1);

	static bool verify_format(std::string const& fmt);
	static bool verify_format(std::wstring const& fmt);

	std::string format(std::string const& format, zone z) const;
	std::wstring format(std::wstring const& format, zone z) const;

	int get_milliseconds() const { return t_ % 1000; }

	time_t get_time_t() const;

	tm get_tm(zone z) const;

#ifdef FZ_WINDOWS
	FILETIME get_filetime() const;
#endif

private:
	int FZ_PRIVATE_SYMBOL compare_slow(datetime const& op) const;

	bool FZ_PRIVATE_SYMBOL clamped();

	enum invalid_t : int64_t {
		invalid = std::numeric_limits<int64_t>::min()
	};

	int64_t t_{invalid};
	accuracy a_{days};
};

class FZ_PUBLIC_SYMBOL duration final
{
public:
	duration() = default;

	int64_t get_days() const { return ms_ / 1000 / 3600 / 24; }
	int64_t get_hours() const { return ms_ / 1000 / 3600; }
	int64_t get_minutes() const { return ms_ / 1000 / 60; }
	int64_t get_seconds() const { return ms_ / 1000; }
	int64_t get_milliseconds() const { return ms_; }

	static duration from_minutes(int64_t m) {
		return duration(m * 1000 * 60);
	}
	static duration from_seconds(int64_t m) {
		return duration(m * 1000);
	}
	static duration from_milliseconds(int64_t m) {
		return duration(m);
	}

	duration& operator-=(duration const& op) {
		ms_ -= op.ms_;
		return *this;
	}

	duration operator-() const {
		return duration(-ms_);
	}

	explicit operator bool() const {
		return ms_ != 0;
	}

	bool operator<(duration const& op) const { return ms_ < op.ms_; }
	bool operator<=(duration const& op) const { return ms_ <= op.ms_; }
	bool operator>(duration const& op) const { return ms_ > op.ms_; }
	bool operator>=(duration const& op) const { return ms_ >= op.ms_; }

	friend duration operator-(duration const& a, duration const& b);
private:
	explicit duration(int64_t ms) : ms_(ms) {}

	int64_t ms_{};
};

inline duration operator-(duration const& a, duration const& b)
{
	return duration(a) -= b;
}


duration FZ_PUBLIC_SYMBOL operator-(datetime const& a, datetime const& b);

class FZ_PUBLIC_SYMBOL monotonic_clock final
{
public:
	monotonic_clock() = default;
	monotonic_clock(monotonic_clock const&) = default;
	monotonic_clock(monotonic_clock &&) noexcept = default;
	monotonic_clock& operator=(monotonic_clock const&) = default;
	monotonic_clock& operator=(monotonic_clock &&) noexcept = default;

	monotonic_clock const operator+(duration const& d) const
	{
		return monotonic_clock(*this) += d;
	}

private:
	typedef std::chrono::steady_clock clock_type;
	static_assert(std::chrono::steady_clock::is_steady, "Nonconforming stdlib, your steady_clock isn't steady");

public:
	static monotonic_clock now() {
		return monotonic_clock(clock_type::now());
	}

	explicit operator bool() const {
		return t_ != clock_type::time_point();
	}

	monotonic_clock& operator+=(duration const& d)
	{
		t_ += std::chrono::milliseconds(d.get_milliseconds());
		return *this;
	}

private:
	explicit monotonic_clock(clock_type::time_point const& t)
		: t_(t)
	{}

	clock_type::time_point t_;

	friend duration operator-(monotonic_clock const& a, monotonic_clock const& b);
	friend bool operator==(monotonic_clock const& a, monotonic_clock const& b);
	friend bool operator<(monotonic_clock const& a, monotonic_clock const& b);
	friend bool operator<=(monotonic_clock const& a, monotonic_clock const& b);
	friend bool operator>(monotonic_clock const& a, monotonic_clock const& b);
	friend bool operator>=(monotonic_clock const& a, monotonic_clock const& b);
};

inline duration operator-(monotonic_clock const& a, monotonic_clock const& b)
{
	return duration::from_milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(a.t_ - b.t_).count());
}

inline bool operator==(monotonic_clock const& a, monotonic_clock const& b)
{
	return a.t_ == b.t_;
}

inline bool operator<(monotonic_clock const& a, monotonic_clock const& b)
{
	return a.t_ < b.t_;
}

inline bool operator<=(monotonic_clock const& a, monotonic_clock const& b)
{
	return a.t_ <= b.t_;
}

inline bool operator>(monotonic_clock const& a, monotonic_clock const& b)
{
	return a.t_ > b.t_;
}

inline bool operator>=(monotonic_clock const& a, monotonic_clock const& b)
{
	return a.t_ >= b.t_;
}

}

#endif
