#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <new>

#include "../debug.hpp"

namespace spaceless {

template<typename T>
class mpsc_queue {
public:
	explicit mpsc_queue(size_t capacity) : capacity_(capacity), mask_(capacity - 1), slots_(std::make_unique<Slot[]>(capacity)) {
		ASSERT(capacity && (capacity & mask_) == 0, "Capacity must be a power of 2", capacity, mask_);

		for (size_t i = 0; i < capacity; ++i)
			slots_[i].sequence.store(i, std::memory_order_relaxed);
	}
	
	~mpsc_queue() { while (try_dequeue()); }

	mpsc_queue(const mpsc_queue &) = delete;
	mpsc_queue& operator=(const mpsc_queue &) = delete;
	mpsc_queue(mpsc_queue &&) = delete;
	mpsc_queue& operator=(mpsc_queue &&) = delete;

	template<typename ...Args>
	void enqueue(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		size_t pos = tail_.fetch_add(1, std::memory_order_relaxed), seq;
		Slot &slot = slots_[pos & mask_];

		while ((seq = slot.sequence.load(std::memory_order_acquire)) != pos)
			slot.sequence.wait(seq, std::memory_order_acquire);

		slot.emplace(pos + 1, std::forward<Args>(args)...);
	}

	std::optional<T> try_dequeue() noexcept(std::is_nothrow_move_constructible_v<T>) {
		Slot &slot = slots_[head_ & mask_];

		if (slot.sequence.load(std::memory_order_acquire) != head_ + 1) return std::nullopt;

		std::optional<T> result(std::move(*slot.data()));
		slot.reset(head_++ + capacity_);

		return result;
	}

	bool approximate_empty() const noexcept { return !approximate_size(); }
	bool approximate_full() const noexcept { return approximate_size() >= capacity_; }
	size_t approximate_size() const noexcept { return tail_.load(std::memory_order_acquire) - head_; }
	size_t capacity() const noexcept { return capacity_; }

private:

	struct alignas(std::hardware_destructive_interference_size) Slot {
		std::atomic_size_t sequence{ 0 };
		alignas(T) std::array<std::byte, sizeof(T)> storage{};

		T *data() noexcept {
			return std::launder(reinterpret_cast<T *>(storage.data()));
		}

		const T *data() const noexcept {
			return std::launder(reinterpret_cast<const T *>(storage.data()));
		}

		template<typename ...Args>
		void emplace(size_t seq, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
			std::construct_at(reinterpret_cast<T *>(storage.data()), std::forward<Args>(args)...);
			sequence.store(seq, std::memory_order_release);
			sequence.notify_one();
		}

		void reset(size_t seq) noexcept {
			std::destroy_at(data());
			sequence.store(seq, std::memory_order_release);
			sequence.notify_one();
		}
	};

	alignas(std::hardware_destructive_interference_size) std::atomic_size_t tail_{ 0 };
	alignas(std::hardware_destructive_interference_size) size_t head_{ 0 };

	const size_t capacity_;
	const size_t mask_;
	std::unique_ptr<Slot[]> slots_;
};

}
